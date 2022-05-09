////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#include "graphics/material_cache.h"
#include "graphics/opengl/opengl_buffer.h"
#include "graphics/opengl/opengl_frame_buffer.h"
#include "graphics/opengl/opengl_material.h"
#include "graphics/opengl/opengl_render_target.h"
#include "graphics/opengl/opengl_uniform.h"
#include "graphics/render_graph/render_graph.h"
#include "graphics/render_queue_builder.h"
#include "graphics/renderer.h"

namespace iris
{

/**
 * Implementation of Renderer for OpenGL.
 */
class OpenGLRenderer : public Renderer
{
  public:
    /**
     * Construct a new OpenGLRenderer.
     *
     * @param width
     *   Width of window being rendered to.
     *
     * @param height
     *   Height of window being rendered to.
     */
    OpenGLRenderer(std::uint32_t width, std::uint32_t height);
    ~OpenGLRenderer() override = default;

    /**
     * Set the render passes. These will be executed when render() is called.
     *
     * @param render_passes
     *   Collection of RenderPass objects to render.
     */
    void set_render_passes(const std::deque<RenderPass> &render_passes) override;

    /**
     * Create a RenderTarget with custom dimensions.
     *
     * @param width
     *   Width of render target.
     *
     * @param height
     *   Height of render target.
     *
     * @returns
     *   RenderTarget.
     */
    RenderTarget *create_render_target(std::uint32_t width, std::uint32_t height) override;

  protected:
    // handlers for the supported RenderCommandTypes

    void execute_pass_start(RenderCommand &command) override;

    void execute_draw(RenderCommand &command) override;

    void execute_present(RenderCommand &command) override;

  private:
    // helper aliases to try and simplify the verbose types
    using LightMaterialMap = std::unordered_map<LightType, std::unique_ptr<OpenGLMaterial>>;

    /** Collection of created RenderTarget objects. */
    std::vector<std::unique_ptr<OpenGLRenderTarget>> render_targets_;

    MaterialCache<OpenGLMaterial, RenderGraph *, LightType, bool, bool> materials_;

    /** Width of window being rendered to. */
    std::uint32_t width_;

    /** Height of window being rendered to. */
    std::uint32_t height_;

    /** Buffer for per pass camera data. */
    std::unique_ptr<UBO> camera_data_;

    /** Buffers for per pass entity bone data. */
    std::unordered_map<const RenderEntity *, std::unique_ptr<UBO>> bone_data_;

    /** Buffers for per pass entity model data. */
    std::unordered_map<const RenderEntity *, std::unique_ptr<SSBO>> model_data_;

    /** Buffers for per scene entity instance data. */
    std::unordered_map<const RenderEntity *, std::unique_ptr<SSBO>> instance_data_;

    /** Buffers for per scene texture data. */
    std::unique_ptr<SSBO> texture_table_;

    /** Buffers for per scene cube map data. */
    std::unique_ptr<SSBO> cube_map_table_;

    /** Buffers for per pass light data. */
    std::unordered_map<const Light *, std::unique_ptr<UBO>> light_data_;

    /** Render queue builder object. */
    std::unique_ptr<RenderQueueBuilder> render_queue_builder_;

    std::unordered_map<const RenderPass *, OpenGLFrameBuffer> pass_frame_buffers_;
};

}
