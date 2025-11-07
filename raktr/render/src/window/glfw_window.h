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
    void set_key_callback(KeyCallback callback) override;
    void set_mouse_button_callback(MouseButtonCallback callback) override;
    void set_cursor_pos_callback(CursorPosCallback callback) override;
    void set_scroll_callback(ScrollCallback callback) override;
    
    bool is_fullscreen() const override;
    void set_fullscreen(bool fullscreen) override;

private:
    explicit GLFWWindow(GLFWwindow* window);
    
    // Static GLFW callback wrappers
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    GLFWwindow* _window = nullptr;
    uint32_t _width = 0;
    uint32_t _height = 0;
    
    // User callbacks
    ResizeCallback _resize_callback;
    KeyCallback _key_callback;
    MouseButtonCallback _mouse_button_callback;
    CursorPosCallback _cursor_pos_callback;
    ScrollCallback _scroll_callback;
    
    // Windowed mode state (for restoring from fullscreen)
    int _windowed_x = 0;
    int _windowed_y = 0;
    uint32_t _windowed_width = 0;
    uint32_t _windowed_height = 0;

    static int _instance_count;
    static bool _glfw_initialized;
};

} // namespace raktr::render::detail

#endif // RAKTR_RENDER_GLFW_WINDOW_H
