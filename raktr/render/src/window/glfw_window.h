/*!
 * @file glfw_window.h
 * @brief GLFW implementation of Window interface.
 */

#ifndef RAKTR_RENDER_GLFW_WINDOW_H
#define RAKTR_RENDER_GLFW_WINDOW_H

#include "window/window.h"

struct GLFWwindow; // Forward declaration

namespace raktr::render::detail
{

/*!
 * @brief GLFW-based window implementation.
 * 
 * Uses GLFW 3.4 for cross-platform window management.
 * Supports OpenGL contexts and native window handles for Vulkan/WebGPU.
 */
class GLFWWindow : public Window
{
public:
    /*!
     * @brief Create GLFW window with given configuration.
     * @param config Window configuration.
     * @return Window instance or error code.
     */
    static std::expected<std::unique_ptr<GLFWWindow>, std::error_code>
    create(const WindowConfig& config);

    ~GLFWWindow() override;

    // Window interface implementation
    bool should_close() const override;
    void poll_events() override;
    void swap_buffers() override;
    uint32_t width() const override;
    uint32_t height() const override;
    void* native_handle() const override;

    void set_resize_callback(ResizeCallback callback) override;

private:
    explicit GLFWWindow(GLFWwindow* window);
    
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    GLFWwindow* _window = nullptr;
    uint32_t _width = 0;
    uint32_t _height = 0;
    ResizeCallback _resize_callback;

    static int _instance_count;
    static bool _glfw_initialized;
};

} // namespace raktr::render::detail

#endif // RAKTR_RENDER_GLFW_WINDOW_H
