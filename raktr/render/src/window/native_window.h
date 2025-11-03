/*!
 * @file native_window.h
 * @brief Native window wrapper for external window handles.
 */

#ifndef RAKTR_RENDER_NATIVE_WINDOW_H
#define RAKTR_RENDER_NATIVE_WINDOW_H

#include "window/window.h"

namespace raktr::render::detail
{

/*!
 * @brief Window wrapper for externally-provided native handles.
 * 
 * Wraps an existing platform window handle (HWND, Window, NSWindow*, etc.)
 * provided by external code (e.g., .NET applications, game engines).
 * 
 * CRITICAL: Does NOT own the window handle. Will not destroy it on cleanup.
 * The external code that created the handle is responsible for its lifecycle.
 * 
 * Use case: When embedding Raktr rendering into an existing application that
 * already manages its own window.
 */
class NativeWindow : public Window
{
public:
    /*!
     * @brief Wrap an external native window handle.
     * 
     * @param native_handle Platform-specific window handle (HWND on Windows, etc.)
     * @param width Initial width in pixels
     * @param height Initial height in pixels
     * @return Window instance or error code.
     * 
     * @note The window handle must remain valid for the lifetime of this object.
     * @note This class does NOT take ownership - it will not destroy the handle.
     */
    static std::expected<std::unique_ptr<NativeWindow>, std::error_code>
    create(void* native_handle, uint32_t width, uint32_t height);

    ~NativeWindow() override;

    // Window interface implementation
    bool should_close() const override;
    void poll_events() override;
    void swap_buffers() override;
    uint32_t width() const override;
    uint32_t height() const override;
    void* native_handle() const override;

    /*!
     * @brief Update window dimensions.
     * 
     * Should be called by external code when the window is resized.
     * Since we don't own the window, we can't detect resize events ourselves.
     * 
     * @param width New width in pixels
     * @param height New height in pixels
     */
    void update_dimensions(uint32_t width, uint32_t height);

    /*!
     * @brief Signal that the window should close.
     * 
     * Called by external code to notify Raktr that the window is closing.
     */
    void request_close();

private:
    NativeWindow(void* native_handle, uint32_t width, uint32_t height);

    void* _native_handle = nullptr;
    uint32_t _width = 0;
    uint32_t _height = 0;
    bool _should_close = false;
};

} // namespace raktr::render::detail

#endif // RAKTR_RENDER_NATIVE_WINDOW_H
