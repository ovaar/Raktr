/*!
 * @file window.h
 * @brief Cross-platform window abstraction for graphics contexts.
 */

#ifndef RAKTR_RENDER_WINDOW_H
#define RAKTR_RENDER_WINDOW_H

#include "render_error.h"
#include <expected>
#include <system_error>
#include <cstdint>
#include <memory>

namespace raktr::render
{

/*!
 * @brief Configuration for window creation.
 */
struct WindowConfig
{
    uint32_t width = 1280;
    uint32_t height = 720;
    const char* title = "Raktr";
    bool resizable = true;
    bool fullscreen = false;
};

/*!
 * @brief Abstract window interface for cross-platform window management.
 * 
 * Provides platform-agnostic window creation, event handling, and surface
 * management for graphics APIs (OpenGL, Vulkan, DirectX, WebGPU).
 */
class Window
{
public:
    virtual ~Window() = default;

    // Non-copyable, moveable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept = default;
    Window& operator=(Window&&) noexcept = default;

    /*!
     * @brief Check if the window should close.
     * @return True if close was requested (e.g., X button clicked).
     */
    virtual bool should_close() const = 0;

    /*!
     * @brief Poll and process window events.
     * 
     * Should be called once per frame to handle input, resize, etc.
     */
    virtual void poll_events() = 0;

    /*!
     * @brief Swap front and back buffers (present frame).
     * 
     * For OpenGL contexts. Other APIs (Vulkan, WebGPU) handle swapping
     * through their own swapchain mechanisms.
     */
    virtual void swap_buffers() = 0;

    /*!
     * @brief Get current window width in pixels.
     */
    virtual uint32_t width() const = 0;

    /*!
     * @brief Get current window height in pixels.
     */
    virtual uint32_t height() const = 0;

    /*!
     * @brief Get native platform window handle.
     * 
     * Returns platform-specific handle for graphics API initialization:
     * - Windows: HWND
     * - Linux X11: Window (XID)
     * - Linux Wayland: wl_surface*
     * - macOS: NSWindow*
     * 
     * @return Opaque pointer to native window handle.
     */
    virtual void* native_handle() const = 0;

protected:
    Window() = default;
};

/*!
 * @brief Factory function to create a window.
 * 
 * Creates platform-specific window implementation (GLFW, SDL, native).
 * 
 * @param config Window configuration.
 * @return Window instance or error code.
 * 
 * @example
 * WindowConfig config;
 * config.width = 1920;
 * config.height = 1080;
 * config.title = "My Game";
 * 
 * auto window_result = create_window(config);
 * if (!window_result) {
 *     // Handle error
 *     return;
 * }
 * auto& window = *window_result;
 * 
 * while (!window->should_close()) {
 *     window->poll_events();
 *     // Render...
 *     window->swap_buffers();
 * }
 */
std::expected<std::unique_ptr<Window>, std::error_code>
create_window(const WindowConfig& config);

} // namespace raktr::render

#endif // RAKTR_RENDER_WINDOW_H
