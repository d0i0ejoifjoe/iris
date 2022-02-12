////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#include "graphics/metal/metal_renderer.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <QuartzCore/QuartzCore.h>

#include "core/error_handling.h"
#include "core/macos/macos_ios_utility.h"
#include "core/matrix4.h"
#include "core/vector3.h"
#include "graphics/constant_buffer_writer.h"
#include "graphics/lights/lighting_rig.h"
#include "graphics/mesh_manager.h"
#include "graphics/metal/metal_constant_buffer.h"
#include "graphics/metal/metal_cube_map.h"
#include "graphics/metal/metal_default_constant_buffer_types.h"
#include "graphics/metal/metal_material.h"
#include "graphics/metal/metal_mesh.h"
#include "graphics/metal/metal_render_target.h"
#include "graphics/metal/metal_texture.h"
#include "graphics/render_entity.h"
#include "graphics/render_graph/post_processing_node.h"
#include "graphics/render_graph/sky_box_node.h"
#include "graphics/render_graph/texture_node.h"
#include "graphics/render_queue_builder.h"
#include "graphics/render_target.h"
#include "graphics/window.h"
#include "log/log.h"

namespace
{

/**
 * Helper function to create a render encoder for render pass.
 *
 * @param colour
 *   Metal texture to write colour data to.
 *
 * @param depth
 *   Metal texture to write depth data to.
 *
 * @param descriptor
 *   Descriptor for render pass.
 *
 * @param depth_stencil_state
 *   State for depth/stencil.
 *
 * @param commands_buffer
 *   Command buffer to create encoder from.
 *
 * @returns
 *   RenderCommandEncoder object for a render pass.
 */
id<MTLRenderCommandEncoder> create_render_encoder(
    id<MTLTexture> colour,
    id<MTLTexture> depth,
    MTLRenderPassDescriptor *descriptor,
    const id<MTLDepthStencilState> depth_stencil_state,
    id<MTLCommandBuffer> command_buffer)
{
    descriptor.colorAttachments[0].texture = colour;
    descriptor.depthAttachment.texture = depth;

    const auto render_encoder = [command_buffer renderCommandEncoderWithDescriptor:descriptor];
    [render_encoder setDepthStencilState:depth_stencil_state];
    [render_encoder setFrontFacingWinding:MTLWindingCounterClockwise];
    [render_encoder setCullMode:MTLCullModeBack];
    [render_encoder setTriangleFillMode:MTLTriangleFillModeFill];

    return render_encoder;
}

/**
 * Helper function to set constant data for a render pass.
 *
 * @param render_encoder
 *   Render encoder for current pass.
 *
 * @param constant_buffer
 *   The constant buffer to write to.
 *
 * @param camera
 *   Camera for current render pass.
 *
 * @param light_data
 *   Date for the current render pass light.
 *
 * @param entity
 *   Entity being rendered.
 *
 * @param shadow_map
 *   Optional RenderTarget for shadow map.
 *
 * @param light
 *   Light for current render pass.
 */
void set_constant_data(
    id<MTLRenderCommandEncoder> render_encoder,
    iris::MetalConstantBuffer &constant_buffer,
    const iris::Camera *camera,
    const iris::RenderEntity *entity,
    const iris::RenderTarget *shadow_map,
    const iris::Light *light)
{
    // this matrix is used to translate projection matrices from engine NDC to
    // metal NDC
    static const iris::Matrix4 metal_translate{
        {{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f}}};

    // set shadow map specific data, if it is present
    if ((shadow_map != nullptr) && (light->type() == iris::LightType::DIRECTIONAL))
    {
        const auto *directional_light = static_cast<const iris::DirectionalLight *>(light);
        iris::DirectionalLightConstantBuffer light_consant_buffer{};

        light_consant_buffer.proj =
            iris::Matrix4::transpose(metal_translate * directional_light->shadow_camera().projection());
        light_consant_buffer.view = iris::Matrix4::transpose(directional_light->shadow_camera().view());

        [render_encoder setVertexBytes:static_cast<const void *>(&light_consant_buffer)
                                length:sizeof(light_consant_buffer)
                               atIndex:2];

        [render_encoder setFragmentBytes:static_cast<const void *>(&light_consant_buffer)
                                  length:sizeof(light_consant_buffer)
                                 atIndex:1];
    }

    iris::ConstantBufferWriter writer(constant_buffer);

    writer.write(metal_translate * camera->projection());
    writer.write(camera->view());
    writer.write(entity->transform());
    writer.write(entity->normal_transform());

    // write all the bone data and ensure we advance past the end of the array
    // note that the transposing of the bone matrices is done by the shader
    writer.write(entity->skeleton().transforms());
    writer.advance(
        sizeof(iris::DefaultConstantBuffer::bones) - entity->skeleton().transforms().size() * sizeof(iris::Matrix4));

    writer.write(camera->position());
    writer.write(0.0f);

    writer.write(light->colour_data());
    writer.write(light->world_space_data());
    writer.write(light->attenuation_data());

    writer.write(0.0f);

    [render_encoder setVertexBuffer:constant_buffer.handle() offset:0 atIndex:1];
    [render_encoder setFragmentBuffer:constant_buffer.handle() offset:0 atIndex:0];
}

/**
 * Helper function to bind all textures for a material.
 *
 * @param render_encoder
 *   Encoder for current render pass.
 *
 * @param material
 *   Material to bind textures for.
 *
 * @param shadow_map
 *   Optional RenderTarget for shadow_map.
 *
 * @param shadow_sampler
 *   Sampler to use for shadow map.
 *
 * @param sky_box
 *   Optional CubeMap for sky box.
 *
 * @param sky_box_sampler
 *   Sampler to use for sky box.
 */
void bind_textures(
    id<MTLRenderCommandEncoder> render_encoder,
    const iris::Material *material,
    const iris::RenderTarget *shadow_map,
    id<MTLSamplerState> shadow_sampler,
    const iris::CubeMap *sky_box,
    id<MTLSamplerState> sky_box_sampler)
{
    // bind shadow map if present
    if (shadow_map != nullptr)
    {
        const auto *metal_texture = static_cast<const iris::MetalTexture *>(shadow_map->depth_texture());
        [render_encoder setFragmentTexture:metal_texture->handle() atIndex:0];
        [render_encoder setFragmentSamplerState:shadow_sampler atIndex:0];
    }

    if (sky_box != nullptr)
    {
        const auto *metal_sky_box = static_cast<const iris::MetalCubeMap *>(sky_box);
        [render_encoder setFragmentTexture:metal_sky_box->handle() atIndex:1];
        [render_encoder setFragmentSamplerState:sky_box_sampler atIndex:1];
    }

    // bind all textures in material
    const auto textures = material->textures();
    for (auto i = 0u; i < textures.size(); ++i)
    {
        const auto *metal_texture = static_cast<const iris::MetalTexture *>(textures[i]);
        [render_encoder setVertexTexture:metal_texture->handle() atIndex:i];
        [render_encoder setFragmentTexture:metal_texture->handle() atIndex:i + 2];
    }
}

}

namespace iris
{

MetalRenderer::MetalRenderer(std::uint32_t width, std::uint32_t height)
    : Renderer()
    , width_(width)
    , height_(height)
    , command_queue_()
    , descriptor_()
    , drawable_()
    , command_buffer_()
    , depth_stencil_state_()
    , render_encoder_()
    , current_frame_(0u)
    , frames_()
    , render_encoders_()
    , render_targets_()
    , materials_()
    , default_depth_buffer_()
    , shadow_sampler_()
    , sky_box_sampler_()
{
    const auto *device = iris::core::utility::metal_device();

    command_queue_ = [device newCommandQueue];
    ensure(command_queue_ != nullptr, "could not create command queue");

    const auto scale = 2;

    // create and setup descriptor for depth texture
    auto *texture_description = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                   width:width * scale
                                                                                  height:height * scale
                                                                               mipmapped:NO];
    [texture_description setResourceOptions:MTLResourceStorageModePrivate];
    [texture_description setUsage:MTLTextureUsageRenderTarget];

    // create descriptor for depth checking
    auto *depth_stencil_descriptor = [MTLDepthStencilDescriptor new];
    [depth_stencil_descriptor setDepthCompareFunction:MTLCompareFunctionLessEqual];
    [depth_stencil_descriptor setDepthWriteEnabled:YES];

    // create depth state
    depth_stencil_state_ = [device newDepthStencilStateWithDescriptor:depth_stencil_descriptor];

    // create and setup a render pass descriptor
    descriptor_ = [MTLRenderPassDescriptor renderPassDescriptor];
    [[[descriptor_ colorAttachments] objectAtIndexedSubscript:0] setLoadAction:MTLLoadActionClear];
    [[[descriptor_ colorAttachments] objectAtIndexedSubscript:0]
        setClearColor:MTLClearColorMake(0.39f, 0.58f, 0.93f, 1.0f)];
    [[[descriptor_ colorAttachments] objectAtIndexedSubscript:0] setStoreAction:MTLStoreActionStore];
    [[[descriptor_ colorAttachments] objectAtIndexedSubscript:0] setTexture:nullptr];
    [[descriptor_ depthAttachment] setTexture:nullptr];
    [[descriptor_ depthAttachment] setClearDepth:1.0f];
    [[descriptor_ depthAttachment] setLoadAction:MTLLoadActionClear];
    [[descriptor_ depthAttachment] setStoreAction:MTLStoreActionStore];

    // create default depth buffer
    default_depth_buffer_ = std::make_unique<MetalTexture>(DataBuffer{}, width * 2u, height * 2u, TextureUsage::DEPTH);

    render_encoder_ = nullptr;

    // create sampler for shadow maps
    MTLSamplerDescriptor *shadow_sampler_descriptor = [MTLSamplerDescriptor new];
    shadow_sampler_descriptor.rAddressMode = MTLSamplerAddressModeClampToBorderColor;
    shadow_sampler_descriptor.sAddressMode = MTLSamplerAddressModeClampToBorderColor;
    shadow_sampler_descriptor.tAddressMode = MTLSamplerAddressModeClampToBorderColor;
    shadow_sampler_descriptor.borderColor = MTLSamplerBorderColorOpaqueWhite;
    shadow_sampler_descriptor.minFilter = MTLSamplerMinMagFilterLinear;
    shadow_sampler_descriptor.magFilter = MTLSamplerMinMagFilterLinear;
    shadow_sampler_descriptor.mipFilter = MTLSamplerMipFilterNotMipmapped;
    shadow_sampler_ = [device newSamplerStateWithDescriptor:shadow_sampler_descriptor];

    // create sampler for sky box
    MTLSamplerDescriptor *sky_box_sampler_descriptor = [MTLSamplerDescriptor new];
    sky_box_sampler_descriptor.minFilter = MTLSamplerMinMagFilterNearest;
    sky_box_sampler_descriptor.magFilter = MTLSamplerMinMagFilterNearest;
    sky_box_sampler_ = [device newSamplerStateWithDescriptor:sky_box_sampler_descriptor];
}

MetalRenderer::~MetalRenderer()
{
    // we need to wait for any inflight frames to have finished being rendered
    // by the GPU before we can destruct
    // this line very deliberately creates an unnamed scoped_lock, this
    // ensures the locks for all frames will be waited on, acquired and then
    // immediately released
    std::scoped_lock{frames_[0].lock, frames_[1].lock, frames_[2].lock};
}

void MetalRenderer::set_render_passes(const std::vector<RenderPass> &render_passes)
{
    render_passes_ = render_passes;

    // check if pass has a sky box and if so create a cube for it and add it to the scene
    for (auto &pass : render_passes_)
    {
        if (pass.sky_box != nullptr)
        {
            auto *scene = pass.scene;

            auto *sky_box_rg = scene->create_render_graph();
            sky_box_rg->set_render_node<SkyBoxNode>(pass.sky_box);

            scene->create_entity(sky_box_rg, Root::mesh_manager().cube({}), Transform({}, {}, {0.5f}));
        }
    }

    // add a post processing pass

    // find the pass which renders to the screen
    auto final_pass = std::find_if(
        std::begin(render_passes_),
        std::end(render_passes_),
        [](const RenderPass &pass) { return pass.render_target == nullptr; });

    ensure(final_pass != std::cend(render_passes_), "no final pass");

    // deferred creating of render target to ensure this class is full
    // constructed
    if (post_processing_target_ == nullptr)
    {
        post_processing_target_ = create_render_target(width_, height_);
        post_processing_camera_ = std::make_unique<Camera>(CameraType::ORTHOGRAPHIC, width_, height_);
    }

    post_processing_scene_ = std::make_unique<Scene>();

    // create a full screen quad which renders the final stage with the post
    // processing node
    auto *rg = post_processing_scene_->create_render_graph();
    rg->set_render_node<PostProcessingNode>(rg->create<TextureNode>(post_processing_target_->colour_texture()));
    post_processing_scene_->create_entity(
        rg,
        Root::mesh_manager().sprite({}),
        Transform({}, {}, {static_cast<float>(width_), static_cast<float>(height_), 1.0}));

    // wire up this pass
    final_pass->render_target = post_processing_target_;
    render_passes_.emplace_back(post_processing_scene_.get(), post_processing_camera_.get(), nullptr);

    // build the render queue from the provided passes

    RenderQueueBuilder queue_builder(
        [this](RenderGraph *render_graph, RenderEntity *entity, const RenderTarget *target, LightType light_type)
        {
            if (materials_.count(render_graph) == 0u || materials_[render_graph].count(light_type) == 0u)
            {
                materials_[render_graph][light_type] = std::make_unique<MetalMaterial>(
                    render_graph, static_cast<const MetalMesh *>(entity->mesh())->descriptors(), light_type);
            }

            return materials_[render_graph][light_type].get();
        },
        [this](std::uint32_t width, std::uint32_t height) { return create_render_target(width, height); });

    render_queue_ = queue_builder.build(render_passes_);

    // clear all constant data buffers
    for (auto &frame : frames_)
    {
        frame.constant_data_buffers.clear();
    }

    // create a constant data buffer for each draw command
    for (const auto &command : render_queue_)
    {
        if (command.type() == RenderCommandType::DRAW)
        {
            const auto *command_ptr = std::addressof(command);

            for (auto &frame : frames_)
            {
                frame.constant_data_buffers.emplace(command_ptr, sizeof(DefaultConstantBuffer));
            }
        }
    }
}

RenderTarget *MetalRenderer::create_render_target(std::uint32_t width, std::uint32_t height)
{
    const auto scale = Root::window_manager().current_window()->screen_scale();

    render_targets_.emplace_back(std::make_unique<MetalRenderTarget>(
        std::make_unique<MetalTexture>(DataBuffer{}, width * scale, height * scale, TextureUsage::RENDER_TARGET),
        std::make_unique<MetalTexture>(DataBuffer{}, width * scale, height * scale, TextureUsage::DEPTH)));

    return render_targets_.back().get();
}

void MetalRenderer::pre_render()
{
    @autoreleasepool
    {
        // acquire the lock, this will be released when the GPU has finished
        // rendering the frame
        frames_[current_frame_ % 3u].lock.lock();

        const auto layer = core::utility::metal_layer();
#if defined(IRIS_PLATFORM_MACOS)
        // can only disable vsync on macos
        [layer setDisplaySyncEnabled:NO];
#endif
        drawable_ = [layer nextDrawable];
        command_buffer_ = [command_queue_ commandBuffer];

        // create render encoders fresh each frame
        render_encoders_.clear();
    }
}

void MetalRenderer::execute_draw(RenderCommand &command)
{
    const auto *material = static_cast<const MetalMaterial *>(command.material());
    const auto *entity = command.render_entity();
    const auto *mesh = static_cast<const MetalMesh *>(entity->mesh());
    const auto *camera = command.render_pass()->camera;
    const auto *target = command.render_pass()->render_target;

    @autoreleasepool
    {
        // create a render encoder based on the render command target
        if (render_encoders_.count(target) == 0u)
        {
            id<MTLRenderCommandEncoder> encoder;
            if (target == nullptr)
            {
                // no target means render to the window frame buffer
                encoder = create_render_encoder(
                    drawable_.texture,
                    default_depth_buffer_->handle(),
                    descriptor_,
                    depth_stencil_state_,
                    command_buffer_);
            }
            else
            {
                encoder = create_render_encoder(
                    static_cast<const MetalTexture *>(target->colour_texture())->handle(),
                    static_cast<const MetalTexture *>(target->depth_texture())->handle(),
                    descriptor_,
                    depth_stencil_state_,
                    command_buffer_);
            }

            render_encoders_[target] = encoder;
        }
    }

    render_encoder_ = render_encoders_[target];

    const auto frame = current_frame_ % 3u;

    set_constant_data(
        render_encoder_,
        frames_[frame].constant_data_buffers.at(std::addressof(command)),
        camera,
        entity,
        command.shadow_map(),
        command.light());
    bind_textures(
        render_encoder_, material, command.shadow_map(), shadow_sampler_, material->cube_map(), sky_box_sampler_);

    const auto &vertex_buffer = mesh->vertex_buffer();
    const auto &index_buffer = mesh->index_buffer();

    // encode render commands
    [render_encoder_ setRenderPipelineState:material->pipeline_state()];
    [render_encoder_ setVertexBuffer:vertex_buffer.handle() offset:0 atIndex:0];
    [render_encoder_ setCullMode:MTLCullModeNone];

    const auto type =
        entity->primitive_type() == iris::PrimitiveType::TRIANGLES ? MTLPrimitiveTypeTriangle : MTLPrimitiveTypeLine;

    // draw command
    [render_encoder_ drawIndexedPrimitives:type
                                indexCount:index_buffer.element_count()
                                 indexType:MTLIndexTypeUInt32
                               indexBuffer:index_buffer.handle()
                         indexBufferOffset:0];
}

void MetalRenderer::execute_pass_end(RenderCommand &)
{
    // end encoding for pass
    [render_encoder_ endEncoding];
}

void MetalRenderer::execute_present(RenderCommand &)
{
    [command_buffer_ presentDrawable:drawable_];

    // store local copy of frame so it can be correctly accessed via the
    // completion handler
    const auto frame = current_frame_ % 3u;

    // set completion handler for command buffer, when the GPU is finished
    // rendering it will fire the handler, which will unlock the lock, allowing
    // the frame to be rendered to again
    [command_buffer_ addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
      frames_[frame].lock.unlock();
    }];

    [command_buffer_ commit];
}

void MetalRenderer::post_render()
{
    ++current_frame_;
}

}
