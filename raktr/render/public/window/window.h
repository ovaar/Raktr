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
#include <functional>

namespace raktr::render
{

/*!
 * @brief Callback function type for window resize events.
 * @param width New width in pixels.
 * @param height New height in pixels.
 */
using ResizeCallback = std::function<void(uint32_t width, uint32_t height)>;

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
    [[nodiscard]] virtual uint32_t width() const = 0;

    /*!
     * @brief Get current window height in pixels.
     */
    [[nodiscard]] virtual uint32_t height() const = 0;

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
    [[nodiscard]] virtual void* native_handle() const = 0;

    /*!
     * @brief Set callback to be invoked when window is resized.
     * 
     * The callback will be invoked with new dimensions whenever the window
     * is resized by the user or programmatically.
     * 
     * @param callback Function to call on resize, or nullptr to clear callback.
     * 
     * @example
     * window->set_resize_callback([](uint32_t w, uint32_t h) {
     *     // Recreate swapchain, update viewport, etc.
     * });
     */
    virtual void set_resize_callback(ResizeCallback callback) = 0;

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
 * // Set up resize callback to handle window size changes
 * window->set_resize_callback([](uint32_t width, uint32_t height) {
 *     // Recreate swapchain, update viewport, etc.
 *     // Called automatically when user resizes window
 * });
 * 
 * while (!window->should_close()) {
 *     window->poll_events();
 *     // Render...
 *     window->swap_buffers();
 * }
 */
std::expected<std::unique_ptr<Window>, std::error_code>
create_window(const WindowConfig& config);

/*!
 * @brief Wrap an existing native window handle.
 * 
 * Creates a Window instance that wraps an externally-provided platform window
 * handle (HWND on Windows, Window on X11, NSWindow* on macOS, etc.).
 * 
 * CRITICAL: The returned Window does NOT own the handle and will not destroy it.
 * The external code that created the handle is responsible for its lifecycle.
 * 
 * Use case: Embedding Raktr rendering into existing applications (e.g., .NET WPF/WinForms,
 * Qt, game engines) that already manage their own windows.
 * 
 * @param native_handle Platform-specific window handle. Must not be null and must remain valid.
 * @param width Initial window width in pixels. Must be > 0.
 * @param height Initial window height in pixels. Must be > 0.
 * @return Window instance or error code.
 * 
 * @example
 * // From a .NET C# application with P/Invoke:
 * // IntPtr hwnd = myWpfWindow.Handle;
 * // Pass to C++:
 * void* hwnd = ...; // HWND from .NET
 * auto window_result = create_window_from_native(hwnd, 1920, 1080);
 * if (!window_result) {
 *     // Handle error
 *     return;
 * }
 * auto& window = *window_result;
 * 
 * // Use for rendering:
 * auto device = WgpuDevice::create(window.get(), false);
 * 
 * // .NET code handles window events and destruction
 */
std::expected<std::unique_ptr<Window>, std::error_code>
create_window_from_native(void* native_handle, uint32_t width, uint32_t height);

} // namespace raktr::render

#endif // RAKTR_RENDER_WINDOW_H
