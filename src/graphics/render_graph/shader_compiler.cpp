////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#include "graphics/render_graph/shader_compiler.h"

#include <iterator>
#include <sstream>
#include <string>

#include "inja/inja.hpp"

#include "glsl/glsl.h"
#include "hlsl/hlsl.h"
#include "msl/msl.h"

#include "core/colour.h"
#include "core/error_handling.h"
#include "core/exception.h"
#include "core/random.h"
#include "core/root.h"
#include "core/vector3.h"
#include "graphics/default_shader_languages.h"
#include "graphics/lights/lighting_rig.h"
#include "graphics/render_graph/arithmetic_node.h"
#include "graphics/render_graph/blur_node.h"
#include "graphics/render_graph/colour_node.h"
#include "graphics/render_graph/combine_node.h"
#include "graphics/render_graph/component_node.h"
#include "graphics/render_graph/composite_node.h"
#include "graphics/render_graph/conditional_node.h"
#include "graphics/render_graph/invert_node.h"
#include "graphics/render_graph/post_processing/ambient_occlusion_node.h"
#include "graphics/render_graph/post_processing/anti_aliasing_node.h"
#include "graphics/render_graph/post_processing/colour_adjust_node.h"
#include "graphics/render_graph/render_node.h"
#include "graphics/render_graph/sin_node.h"
#include "graphics/render_graph/sky_box_node.h"
#include "graphics/render_graph/texture_node.h"
#include "graphics/render_graph/value_node.h"
#include "graphics/render_graph/vertex_node.h"
#include "graphics/sampler.h"
#include "graphics/texture.h"
#include "graphics/texture_manager.h"

namespace
{

/**
 * Return a shader language specific string.
 *
 * @param language
 *   The language to get string for.
 *
 * @param hlsl
 *   HLSL compiler string.
 *
 * @param glsl
 *   GLSL compiler string.
 *
 * @param metal
 *   Metal compiler string.
 *
 * @returns
 *   Language specific string.
 */
std::string language_string(
    iris::ShaderLanguage language,
    const std::string &hlsl_str,
    const std::string &glsl_str,
    const std::string &metal_str)
{
    switch (language)
    {
        case iris::ShaderLanguage::HLSL: return hlsl_str; break;
        case iris::ShaderLanguage::GLSL: return glsl_str; break;
        case iris::ShaderLanguage::MSL: return metal_str; break;
        default: throw iris::Exception("unsupported shader language");
    }
}

}

namespace iris
{

ShaderCompiler::ShaderCompiler(
    ShaderLanguage language,
    const RenderGraph *render_graph,
    LightType light_type,
    bool render_to_normal_target,
    bool render_to_position_target)
    : language_(language)
    , vertex_stream_()
    , fragment_stream_()
    , fragment_functions_()
    , light_type_(light_type)
    , render_to_normal_target_(render_to_normal_target)
    , render_to_position_target_(render_to_position_target)
{
    render_graph->render_node()->accept(*this);
}

void ShaderCompiler::visit(const RenderNode &node)
{
    // build vertex shader

    const ::inja::json vertex_args{{"is_directional_light", light_type_ == LightType::DIRECTIONAL}};
    vertex_stream_ << ::inja::render(
        language_string(language_, hlsl::render_node_vertex, glsl::render_node_vertex, msl::render_node_vertex),
        vertex_args);

    // build fragment shader

    fragment_functions_.emplace(
        language_string(language_, hlsl::shadow_function, glsl::shadow_function, msl::shadow_function));

    ::inja::json fragment_args{
        {"render_normal", render_to_normal_target_},
        {"render_position", render_to_position_target_},
        {"light_type", static_cast<std::uint32_t>(light_type_)}};

    if (node.colour_input() != nullptr)
    {
        stream_stack_.push(std::stringstream{});
        node.colour_input()->accept(*this);
        fragment_args["fragment_colour"] = stream_stack_.top().str();
        stream_stack_.pop();
    }

    if (node.normal_input() != nullptr)
    {
        stream_stack_.push(std::stringstream{});
        node.normal_input()->accept(*this);
        fragment_args["normal"] = stream_stack_.top().str();
        stream_stack_.pop();
    }

    if (node.ambient_occlusion_input() != nullptr)
    {
        stream_stack_.push(std::stringstream{});
        node.ambient_occlusion_input()->accept(*this);
        fragment_args["ambient_input"] = stream_stack_.top().str();
        stream_stack_.pop();
    }

    fragment_stream_ << ::inja::render(
        language_string(language_, hlsl::render_node_fragment, glsl::render_node_fragment, msl::render_node_fragment),
        fragment_args);
}

void ShaderCompiler::visit(const SkyBoxNode &node)
{
    // build vertex shader

    const ::inja::json vertex_args;
    vertex_stream_ << ::inja::render(
        language_string(language_, hlsl::sky_box_node_vertex, glsl::sky_box_node_vertex, msl::sky_box_node_vertex),
        vertex_args);

    // build fragment shader

    fragment_functions_.emplace(
        language_string(language_, hlsl::shadow_function, glsl::shadow_function, msl::shadow_function));

    const ::inja::json fragment_args{
        {"cube_map_index", node.sky_box()->index()}, {"sampler_index", node.sky_box()->sampler()->index()}};

    fragment_stream_ << ::inja::render(
        language_string(
            language_, hlsl::sky_box_node_fragment, glsl::sky_box_node_fragment, msl::sky_box_node_fragment),
        fragment_args);
}

void ShaderCompiler::visit(const ColourNode &node)
{
    const auto colour = node.colour();

    const ::inja::json args{{"r", colour.r}, {"g", colour.g}, {"b", colour.b}, {"a", colour.a}};

    stream_stack_.top() << ::inja::render(
        language_string(language_, hlsl::colour_node_chunk, glsl::colour_node_chunk, msl::colour_node_chunk), args);
}

void ShaderCompiler::visit(const TextureNode &node)
{
    std::string tex_coord;
    if (node.uv_input() != nullptr)
    {
        stream_stack_.push(std::stringstream{});
        node.uv_input()->accept(*this);
        tex_coord = stream_stack_.top().str();
        stream_stack_.pop();
    }

    const ::inja::json args{
        {"uv_source", static_cast<std::uint32_t>(node.uv_source())},
        {"texture_index", node.texture()->index()},
        {"sampler_index", node.texture()->sampler()->index()},
        {"reciprocal_width", 1.0f / node.texture()->width()},
        {"reciprocal_height", 1.0f / node.texture()->height()},
        {"tex_coord", tex_coord}};

    stream_stack_.top() << ::inja::render(
        language_string(language_, hlsl::texture_node_chunk, glsl::texture_node_chunk, msl::texture_node_chunk), args);
}

void ShaderCompiler::visit(const InvertNode &node)
{
    fragment_functions_.emplace(
        language_string(language_, hlsl::invert_function, glsl::invert_function, msl::invert_function));

    stream_stack_.push(std::stringstream{});
    node.input_node()->accept(*this);

    const ::inja::json args{{"input", stream_stack_.top().str()}};

    stream_stack_.pop();
    stream_stack_.top() << ::inja::render(
        language_string(language_, hlsl::invert_node_chunk, glsl::invert_node_chunk, msl::invert_node_chunk), args);
}

void ShaderCompiler::visit(const BlurNode &node)
{
    fragment_functions_.emplace(
        language_string(language_, hlsl::blur_function, glsl::blur_function, msl::blur_function));

    const ::inja::json args{
        {"texture_index", node.input_node()->texture()->index()},
        {"sampler_index", node.input_node()->texture()->sampler()->index()}};

    stream_stack_.top() << ::inja::render(
        language_string(language_, hlsl::blur_node_chunk, glsl::blur_node_chunk, msl::blur_node_chunk), args);
}

void ShaderCompiler::visit(const CompositeNode &node)
{
    fragment_functions_.emplace(
        language_string(language_, hlsl::composite_function, glsl::composite_function, msl::composite_function));

    std::array<Node *, 4u> nodes{{node.colour1(), node.colour2(), node.depth1(), node.depth2()}};
    std::array<std::string, 4u> node_strs{};

    for (auto i = 0u; i < nodes.size(); ++i)
    {
        stream_stack_.push(std::stringstream{});
        nodes[i]->accept(*this);
        node_strs[i] = stream_stack_.top().str();
        stream_stack_.pop();
    }

    const ::inja::json args{
        {"colour1", node_strs[0]}, {"colour2", node_strs[1]}, {"depth1", node_strs[2]}, {"depth2", node_strs[3]}};

    stream_stack_.top() << ::inja::render(
        language_string(language_, hlsl::composite_node_chunk, glsl::composite_node_chunk, msl::composite_node_chunk),
        args);
}

void ShaderCompiler::visit(const ValueNode<float> &node)
{
    const ::inja::json args{{"value", node.value()}};
    stream_stack_.top() << ::inja::render(
        language_string(
            language_, hlsl::value_node_float_chunk, glsl::value_node_float_chunk, msl::value_node_float_chunk),
        args);
}

void ShaderCompiler::visit(const ValueNode<Vector3> &node)
{
    const auto value = node.value();

    const ::inja::json args{{"x", value.x}, {"y", value.y}, {"z", value.z}};
    stream_stack_.top() << ::inja::render(
        language_string(
            language_, hlsl::value_node_vector3_chunk, glsl::value_node_vector3_chunk, msl::value_node_vector3_chunk),
        args);
}

void ShaderCompiler::visit(const ValueNode<Colour> &node)
{
    const auto value = node.value();

    const ::inja::json args{{"r", value.r}, {"g", value.g}, {"b", value.b}, {"a", value.a}};
    stream_stack_.top() << ::inja::render(
        language_string(
            language_, hlsl::value_node_colour_chunk, glsl::value_node_colour_chunk, msl::value_node_colour_chunk),
        args);
}

void ShaderCompiler::visit(const ArithmeticNode &node)
{
    std::array<Node *, 2u> nodes{{node.value1(), node.value2()}};
    std::array<std::string, 2u> node_strs{};

    for (auto i = 0u; i < nodes.size(); ++i)
    {
        stream_stack_.push(std::stringstream{});
        nodes[i]->accept(*this);
        node_strs[i] = stream_stack_.top().str();
        stream_stack_.pop();
    }

    const ::inja::json args{
        {"operator", static_cast<std::uint32_t>(node.arithmetic_operator())},
        {"value1", node_strs[0]},
        {"value2", node_strs[1]}};

    stream_stack_.top() << ::inja::render(
        language_string(
            language_, hlsl::arithmetic_node_chunk, glsl::arithmetic_node_chunk, msl::arithmetic_node_chunk),
        args);
}

void ShaderCompiler::visit(const ConditionalNode &node)
{
    std::array<Node *, 4u> nodes{
        {node.input_value1(), node.input_value2(), node.output_value1(), node.output_value2()}};
    std::array<std::string, 4u> node_strs{};

    for (auto i = 0u; i < nodes.size(); ++i)
    {
        stream_stack_.push(std::stringstream{});
        nodes[i]->accept(*this);
        node_strs[i] = stream_stack_.top().str();
        stream_stack_.pop();
    }

    const ::inja::json args{
        {"input1", node_strs[0]},
        {"input2", node_strs[1]},
        {"output1", node_strs[2]},
        {"output2", node_strs[3]},
        {"operator", ">"}};

    stream_stack_.top() << ::inja::render(
        language_string(
            language_, hlsl::conditional_node_chunk, glsl::conditional_node_chunk, msl::conditional_node_chunk),
        args);
}

void ShaderCompiler::visit(const ComponentNode &node)
{
    stream_stack_.push(std::stringstream{});
    node.input_node()->accept(*this);
    const auto value = stream_stack_.top().str();
    stream_stack_.pop();

    const ::inja::json args{{"value", value, {"component", node.component()}}};

    stream_stack_.top() << ::inja::render(
        language_string(language_, hlsl::component_node_chunk, glsl::component_node_chunk, msl::component_node_chunk),
        args);
}

void ShaderCompiler::visit(const CombineNode &node)
{
    std::array<Node *, 4u> nodes{{node.value1(), node.value2(), node.value3(), node.value4()}};
    std::array<std::string, 4u> node_strs{};

    for (auto i = 0u; i < nodes.size(); ++i)
    {
        stream_stack_.push(std::stringstream{});
        nodes[i]->accept(*this);
        node_strs[i] = stream_stack_.top().str();
        stream_stack_.pop();
    }

    const ::inja::json args{{"x", node_strs[0]}, {"y", node_strs[1]}, {"z", node_strs[2]}, {"w", node_strs[3]}};

    stream_stack_.top() << ::inja::render(
        language_string(language_, hlsl::combine_node_chunk, glsl::combine_node_chunk, msl::combine_node_chunk), args);
}

void ShaderCompiler::visit(const SinNode &node)
{
    stream_stack_.push(std::stringstream{});
    node.accept(*this);
    const auto value = stream_stack_.top().str();
    stream_stack_.pop();

    const ::inja::json args{{"value", value}};

    stream_stack_.top() << ::inja::render(
        language_string(language_, hlsl::sin_node_chunk, glsl::sin_node_chunk, msl::sin_node_chunk), args);
}

void ShaderCompiler::visit(const VertexNode &node)
{
    const ::inja::json args{
        {"type", static_cast<std::uint32_t>(node.vertex_data_type())}, {"swizzle", node.swizzle().value_or("")}};

    stream_stack_.top() << ::inja::render(
        language_string(language_, hlsl::vertex_node_chunk, glsl::vertex_node_chunk, msl::vertex_node_chunk), args);
}

void ShaderCompiler::visit(const AmbientOcclusionNode &node)
{
    // build vertex shader

    ::inja::json vertex_args;
    vertex_stream_ << ::inja::render(
        language_string(
            language_,
            hlsl::ambient_occlusion_node_vertex,
            glsl::ambient_occlusion_node_vertex,
            msl::ambient_occlusion_node_vertex),
        vertex_args);

    // build fragment shader

    const auto *input_texture = static_cast<const TextureNode *>(node.colour_input())->texture();

    stream_stack_.push(std::stringstream{});
    std::string fragment_colour;
    if (node.colour_input() == nullptr)
    {
        fragment_colour = "input.colour";
    }
    else
    {
        node.colour_input()->accept(*this);
        fragment_colour = stream_stack_.top().str();
    }

    const ::inja::json fragment_args{
        {"fragment_colour", fragment_colour},
        {"width", input_texture->width()},
        {"height", input_texture->height()},
        {"position_texture_index", node.position_texture()->texture()->index()},
        {"position_sampler_index", node.position_texture()->texture()->sampler()->index()},
        {"normal_texture_index", node.normal_texture()->texture()->index()},
        {"normal_sampler_index", node.normal_texture()->texture()->sampler()->index()},
        {"sample_count", node.description().sample_count},
        {"radius", node.description().radius},
        {"bias", node.description().bias}};

    fragment_stream_ << ::inja::render(
        language_string(
            language_,
            hlsl::ambient_occlusion_node_fragment,
            glsl::ambient_occlusion_node_fragment,
            msl::ambient_occlusion_node_fragment),
        fragment_args);
}

void ShaderCompiler::visit(const ColourAdjustNode &node)
{
    // build vertex shader

    ::inja::json vertex_args;
    vertex_stream_ << ::inja::render(
        language_string(
            language_,
            hlsl::colour_adjust_node_vertex,
            glsl::colour_adjust_node_vertex,
            msl::colour_adjust_node_vertex),
        vertex_args);

    // build fragment shader

    stream_stack_.push(std::stringstream{});
    std::string fragment_colour;
    if (node.colour_input() == nullptr)
    {
        fragment_colour = "input.colour";
    }
    else
    {
        node.colour_input()->accept(*this);
        fragment_colour = stream_stack_.top().str();
    }

    const ::inja::json fragment_args{{"fragment_colour", fragment_colour}, {"gamma", 1.0f / node.description().gamma}};
    fragment_stream_ << ::inja::render(
        language_string(
            language_,
            hlsl::colour_adjust_node_fragment,
            glsl::colour_adjust_node_fragment,
            msl::colour_adjust_node_fragment),
        fragment_args);
}

void ShaderCompiler::visit(const AntiAliasingNode &node)
{
    // build vertex shader

    ::inja::json vertex_args;
    vertex_stream_ << ::inja::render(
        language_string(
            language_,
            hlsl::anti_aliasing_node_vertex,
            glsl::anti_aliasing_node_vertex,
            msl::anti_aliasing_node_vertex),
        vertex_args);

    // build fragment shader

    const auto *input_texture = static_cast<const TextureNode *>(node.colour_input())->texture();

    fragment_functions_.emplace(
        language_string(language_, hlsl::rgb_to_luma_function, glsl::rgb_to_luma_function, msl::rgb_to_luma_function));

    stream_stack_.push(std::stringstream{});
    std::string fragment_colour;
    if (node.colour_input() == nullptr)
    {
        fragment_colour = "input.colour";
    }
    else
    {
        node.colour_input()->accept(*this);
        fragment_colour = stream_stack_.top().str();
    }

    const ::inja::json fragment_args{
        {"fragment_colour", fragment_colour},
        {"input_texture_index", input_texture->index()},
        {"input_sampler_index", input_texture->sampler()->index()},
        {"inverse_width", 1.0f / static_cast<float>(input_texture->width())},
        {"inverse_height", 1.0f / static_cast<float>(input_texture->height())}};

    fragment_stream_ << ::inja::render(
        language_string(
            language_,
            hlsl::anti_aliasing_node_fragment,
            glsl::anti_aliasing_node_fragment,
            msl::anti_aliasing_node_fragment),
        fragment_args);
}

std::string ShaderCompiler::vertex_shader() const
{
    std::stringstream strm{};

    if (language_ == ShaderLanguage::GLSL)
    {
        strm << R"(
#version 430 core
precision mediump float;
#extension GL_ARB_bindless_texture : require
        )";
    }

    strm << vertex_stream_.str() << '\n';

    return strm.str();
}

std::string ShaderCompiler::fragment_shader() const
{
    std::stringstream strm{};

    if (language_ == ShaderLanguage::GLSL)
    {
        strm << R"(
#version 430 core
precision mediump float;
#extension GL_ARB_bindless_texture : require
        )";
    }

    for (const auto &function : fragment_functions_)
    {
        strm << function << '\n';
    }

    strm << fragment_stream_.str() << '\n';

    return strm.str();
}

}