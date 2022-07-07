////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "graphics/opengl/opengl_render_target.h"
#include "graphics/render_target_manager.h"

namespace iris
{

/**
 * Implementation of RenderTargetManager for OpenGL.
 */
class OpenGLRenderTargetManager : public RenderTargetManager
{
  public:
    ~OpenGLRenderTargetManager() override = default;

    /**
     * Create a render target the size of the current window.
     *
     * @returns
     *   Pointer to the created render target.
     */
    RenderTarget *create() override;

    /**
     * Create a render target with the specified dimensions.
     *
     * @param width
     *   Width of render target.
     *
     * @param height
     *   Height of render target.
     *
     * @returns
     *   Pointer to the created render target.
     */
    RenderTarget *create(std::uint32_t width, std::uint32_t height) override;

    /**
     * Create a new render target which combines the colour target and depth target of two separate render targets.
     *
     * @param colour_target
     *   Render target to for colour target.
     *
     * @param depth_target
     *   Render target to for depth target.
     *
     * @returns
     *   Pointer to the created render target.
     */
    RenderTarget *create(const RenderTarget *colour_target, const RenderTarget *depth_target) override;

    /** Collection of created render targets. */
  private:
    std::vector<std::unique_ptr<OpenGLRenderTarget>> render_targets_;
};

}
