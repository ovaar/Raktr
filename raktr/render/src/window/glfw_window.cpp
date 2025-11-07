/*!
 * @file glfw_window.cpp
 * @brief GLFW window implementation.
 */

#include "glfw_window.h"
#include "native_window.h"
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace raktr::render
{

    namespace detail
    {

        int  GLFWWindow::_instance_count   = 0;
        bool GLFWWindow::_glfw_initialized = false;

        GLFWWindow::GLFWWindow(GLFWwindow* window)
            : _window(window)
        {
            if (_window)
            {
                int w, h;
                glfwGetFramebufferSize(_window, &w, &h);
                _width  = static_cast<uint32_t>(w);
                _height = static_cast<uint32_t>(h);

                // Save initial windowed position and size
                if (!is_fullscreen())
                {
                    glfwGetWindowPos(_window, &_windowed_x, &_windowed_y);
                    _windowed_width  = _width;
                    _windowed_height = _height;
                }

                // Set user pointer for callback access
                glfwSetWindowUserPointer(_window, this);

                // Set GLFW framebuffer size callback
                glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);
            }
            ++_instance_count;
        }

        GLFWWindow::~GLFWWindow()
        {
            if (_window)
            {
                glfwDestroyWindow(std::exchange(_window, nullptr));
            }

            --_instance_count;
            if (_instance_count == 0 && _glfw_initialized)
            {
                glfwTerminate();
                _glfw_initialized = false;
            }
        }

        std::expected<std::unique_ptr<GLFWWindow>, std::error_code>
        GLFWWindow::create(const WindowConfig& config)
        {
            // Initialize GLFW once
            if (!_glfw_initialized)
            {
                if (!glfwInit())
                {
                    return std::unexpected(make_error_code(RenderError::InitializationFailed));
                }
                _glfw_initialized = true;
            }

            // For WebGPU/Vulkan, don't create OpenGL context
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

            // Create window
            GLFWwindow* window = glfwCreateWindow(
                static_cast<int>(config.width),
                static_cast<int>(config.height),
                config.title,
                config.fullscreen ? glfwGetPrimaryMonitor() : nullptr,
                nullptr);

            if (!window)
            {
                return std::unexpected(make_error_code(RenderError::WindowCreationFailed));
            }

            return std::unique_ptr<GLFWWindow>(new GLFWWindow(window));
        }

        bool GLFWWindow::should_close() const
        {
            return _window ? glfwWindowShouldClose(_window) : true;
        }

        void GLFWWindow::poll_events()
        {
            glfwPollEvents();

            // Update cached dimensions in case of resize
            if (_window)
            {
                int w, h;
                glfwGetFramebufferSize(_window, &w, &h);
                _width  = static_cast<uint32_t>(w);
                _height = static_cast<uint32_t>(h);
            }
        }

        void GLFWWindow::swap_buffers()
        {
            if (_window)
            {
                glfwSwapBuffers(_window);
            }
        }

        uint32_t GLFWWindow::width() const
        {
            return _width;
        }

        uint32_t GLFWWindow::height() const
        {
            return _height;
        }

        void* GLFWWindow::native_handle() const
        {
            if (!_window)
            {
                return nullptr;
            }

#ifdef _WIN32
            return glfwGetWin32Window(_window);
#elif defined(__linux__)
            // For X11
            return reinterpret_cast<void*>(glfwGetX11Window(_window));
#elif defined(__APPLE__)
            return glfwGetCocoaWindow(_window);
#else
            return nullptr;
#endif
        }

        void GLFWWindow::set_resize_callback(ResizeCallback callback)
        {
            _resize_callback = std::move(callback);
        }

        void GLFWWindow::set_key_callback(KeyCallback callback)
        {
            _key_callback = std::move(callback);
            glfwSetKeyCallback(_window, _key_callback ? key_callback : nullptr);
        }

        void GLFWWindow::set_mouse_button_callback(MouseButtonCallback callback)
        {
            _mouse_button_callback = std::move(callback);
            glfwSetMouseButtonCallback(_window, _mouse_button_callback ? mouse_button_callback : nullptr);
        }

        void GLFWWindow::set_cursor_pos_callback(CursorPosCallback callback)
        {
            _cursor_pos_callback = std::move(callback);
            glfwSetCursorPosCallback(_window, _cursor_pos_callback ? cursor_pos_callback : nullptr);
        }

        void GLFWWindow::set_scroll_callback(ScrollCallback callback)
        {
            _scroll_callback = std::move(callback);
            glfwSetScrollCallback(_window, _scroll_callback ? scroll_callback : nullptr);
        }

        void GLFWWindow::framebuffer_size_callback(GLFWwindow* window, int width, int height)
        {
            auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self)
            {
                // Update cached dimensions
                self->_width  = static_cast<uint32_t>(width);
                self->_height = static_cast<uint32_t>(height);

                // Invoke user callback if set
                if (self->_resize_callback)
                {
                    self->_resize_callback(self->_width, self->_height);
                }
            }
        }

        void GLFWWindow::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self && self->_key_callback)
            {
                self->_key_callback(key, scancode, action, mods);
            }
        }

        void GLFWWindow::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
        {
            auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self && self->_mouse_button_callback)
            {
                self->_mouse_button_callback(button, action, mods);
            }
        }

        void GLFWWindow::cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
        {
            auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self && self->_cursor_pos_callback)
            {
                self->_cursor_pos_callback(xpos, ypos);
            }
        }

        void GLFWWindow::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
        {
            auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (self && self->_scroll_callback)
            {
                self->_scroll_callback(xoffset, yoffset);
            }
        }

        bool GLFWWindow::is_fullscreen() const
        {
            if (!_window)
            {
                return false;
            }

            return glfwGetWindowMonitor(_window) != nullptr;
        }

        void GLFWWindow::set_fullscreen(bool fullscreen)
        {
            if (!_window)
            {
                return;
            }

            // Check if already in desired state
            if (is_fullscreen() == fullscreen)
            {
                return;
            }

            if (fullscreen)
            {
                // Save windowed position and size before going fullscreen
                glfwGetWindowPos(_window, &_windowed_x, &_windowed_y);
                glfwGetWindowSize(_window, reinterpret_cast<int*>(&_windowed_width), reinterpret_cast<int*>(&_windowed_height));

                // Get primary monitor and its video mode
                GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

                // Switch to fullscreen
                glfwSetWindowMonitor(_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else
            {
                // Restore windowed mode with saved position and size
                glfwSetWindowMonitor(_window, nullptr, _windowed_x, _windowed_y, _windowed_width, _windowed_height, GLFW_DONT_CARE);
            }
        }

    } // namespace detail

    // Factory function implementations
    std::expected<std::unique_ptr<Window>, std::error_code>
    create_window(const WindowConfig& config)
    {
        auto result = detail::GLFWWindow::create(config);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        // Move unique_ptr<GLFWWindow> to unique_ptr<Window>
        return std::unique_ptr<Window>(result->release());
    }

    std::expected<std::unique_ptr<Window>, std::error_code>
    create_window_from_native(void* native_handle, uint32_t width, uint32_t height)
    {
        auto result = detail::NativeWindow::create(native_handle, width, height);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        // Move unique_ptr<NativeWindow> to unique_ptr<Window>
        return std::unique_ptr<Window>(result->release());
    }

} // namespace raktr::render
