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

int GLFWWindow::_instance_count = 0;
bool GLFWWindow::_glfw_initialized = false;

GLFWWindow::GLFWWindow(GLFWwindow* window)
    : _window(window)
{
    if (_window)
    {
        int w, h;
        glfwGetFramebufferSize(_window, &w, &h);
        _width = static_cast<uint32_t>(w);
        _height = static_cast<uint32_t>(h);
    }
    ++_instance_count;
}

GLFWWindow::~GLFWWindow()
{
    if (_window)
    {
        glfwDestroyWindow(_window);
        _window = nullptr;
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
        nullptr
    );

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
        _width = static_cast<uint32_t>(w);
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
