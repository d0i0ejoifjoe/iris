#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <queue>

#include "core/camera.h"
#include "events/event.h"
#include "graphics/pipeline.h"
#include "graphics/render_system.h"
#include "graphics/render_target.h"

namespace iris
{

/**
 * Class representing a native window.
 */
class Window
{
  public:
    /**
     * Create and display a new native window.
     *
     * @param width
     *   Width of the window.
     *
     * @param height
     *   Height of the window.
     */
    Window(std::uint32_t with, std::uint32_t height);

    ~Window();

    /** Disabled */
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    /**
     * Render a Pipeline. Afterwards the default screen target is displayed.
     *
     * @param pipeline
     *   Pipeline to execute.
     */
    void render(const Pipeline &pipeline) const;

    /**
     * Pump the next user input event. Result will be empty if there are no
     * new events.
     *
     * @returns
     *   Optional event.
     */
    std::optional<Event> pump_event();

    /**
     * Get the width of the window.
     *
     * @returns
     *   Window width.
     */
    std::uint32_t width() const;

    /**
     * Get the height of the window.
     *
     * @returns
     *   Window height.
     */
    std::uint32_t height() const;

    /**
     * Get a pointer to the screen render target.
     *
     * @returns
     *   Screen render target.
     */
    RenderTarget *screen_target() const;

    /**
     * Get the natural scale for the screen. This value reflects the scale
     * factor needed to convert from the default logical coordinate space into
     * the device coordinate space of this screen.
     *
     * @returns
     *   Screen scale factor.
     */
    static std::uint32_t screen_scale();

  private:
    /** Window width. */
    std::uint32_t width_;

    /** Window height. */
    std::uint32_t height_;

    /** Render system for window. */
    std::unique_ptr<RenderSystem> render_system_;

    /** Render target fro screen. */
    std::unique_ptr<RenderTarget> screen_target_;

    /** Pointer to implementation. */
    struct implementation;
    std::unique_ptr<implementation> impl_;
};

}
