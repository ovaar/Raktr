/*!
 * @file native_window.cpp
 * @brief Native window wrapper implementation.
 */

#include "native_window.h"

namespace raktr::render::detail
{

NativeWindow::NativeWindow(void* native_handle, uint32_t width, uint32_t height)
    : _native_handle(native_handle)
    , _width(width)
    , _height(height)
    , _should_close(false)
{
}

NativeWindow::~NativeWindow()
{
    // CRITICAL: Do NOT destroy the window handle!
    // We don't own it - external code is responsible for cleanup.
    _native_handle = nullptr;
}

std::expected<std::unique_ptr<NativeWindow>, std::error_code>
NativeWindow::create(void* native_handle, uint32_t width, uint32_t height)
{
    if (!native_handle)
    {
        return std::unexpected(make_error_code(RenderError::WindowCreationFailed));
    }

    if (width == 0 || height == 0)
    {
        return std::unexpected(make_error_code(RenderError::InvalidOperation));
    }

    return std::unique_ptr<NativeWindow>(new NativeWindow(native_handle, width, height));
}

bool NativeWindow::should_close() const
{
    return _should_close;
}

void NativeWindow::poll_events()
{
    // External code handles event polling for the native window.
    // This is a no-op for NativeWindow.
}

void NativeWindow::swap_buffers()
{
    // External code handles buffer swapping for the native window.
    // This is a no-op for NativeWindow.
}

uint32_t NativeWindow::width() const
{
    return _width;
}

uint32_t NativeWindow::height() const
{
    return _height;
}

void* NativeWindow::native_handle() const
{
    return _native_handle;
}

void NativeWindow::update_dimensions(uint32_t width, uint32_t height)
{
    _width = width;
    _height = height;
    
    // Invoke resize callback if set
    if (_resize_callback)
    {
        _resize_callback(width, height);
    }
}

void NativeWindow::request_close()
{
    _should_close = true;
}

void NativeWindow::set_resize_callback(ResizeCallback callback)
{
    _resize_callback = std::move(callback);
}

void NativeWindow::set_key_callback(KeyCallback callback)
{
    _key_callback = std::move(callback);
}

void NativeWindow::set_mouse_button_callback(MouseButtonCallback callback)
{
    _mouse_button_callback = std::move(callback);
}

void NativeWindow::set_cursor_pos_callback(CursorPosCallback callback)
{
    _cursor_pos_callback = std::move(callback);
}

void NativeWindow::set_scroll_callback(ScrollCallback callback)
{
    _scroll_callback = std::move(callback);
}

bool NativeWindow::is_fullscreen() const
{
    return _is_fullscreen;
}

void NativeWindow::set_fullscreen(bool fullscreen)
{
    // NativeWindow doesn't own the window, so it can't actually switch modes.
    // We just update the flag. External code is responsible for the actual switch.
    _is_fullscreen = fullscreen;
}

} // namespace raktr::render
