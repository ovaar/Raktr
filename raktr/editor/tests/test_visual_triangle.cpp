/*!
 * @file test_visual_triangle.cpp
 * @brief Visual test for triangle rendering - keeps window open for visual verification.
 *
 * This is not an automated test. Run manually to see the rendered triangle.
 * Press ESC or close the window to exit.
 */
#include "aspect_ratio.h"
#include "buffer.h"
#include "input/input_system.h" // From engine module
#include "math/transform.h"
#include "math/transform_types.h"
#include "render_context.h"
#include "scene/camera.h" // From engine module
#include "window/window.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <thread>

using namespace raktr::render;

TEST(VisualTest, DISABLED_ManualRenderTriangle)
{
    // Create window FIRST
    WindowConfig window_config;
    window_config.width     = 800;
    window_config.height    = 600;
    window_config.title     = "WebGPU Triangle Test";
    window_config.resizable = true;

    auto window_result = create_window(window_config);
    ASSERT_TRUE(window_result.has_value()) << "Failed to create window";

    auto& window = window_result.value();

    // Create render context with the window (caller owns window)
    RenderConfig render_config;
    render_config.backend           = BackendType::WebGPU;
    render_config.enable_validation = false;

    auto render_context = create_render_context();
    ASSERT_NE(render_context, nullptr);

    auto render_ctx_result = render_context->initialize(render_config, window.get());
    ASSERT_TRUE(render_ctx_result.has_value()) << "Failed to initialize RenderContext with WebGPU backend";

    // Create WebGPU device
    auto device = render_context->device();
    ASSERT_NE(device, nullptr) << "Failed to get WebGPU device";

    // Create triangle vertex buffer
    // Positions in NDC space: top center, bottom left, bottom right
    // clang-format off
    float vertices[] = {
         0.0f,  0.6f, 0.0f,  // Top (will be greenish due to shader)
        -0.6f, -0.6f, 0.0f,  // Bottom-left (will be bluish)
         0.6f, -0.6f, 0.0f   // Bottom-right (will be reddish)
    };
    // clang-format on

    auto vertex_data   = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device->create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value()) << "Failed to create vertex buffer";

    // Create index buffer
    uint32_t indices[]    = { 0, 1, 2 };
    auto     index_data   = std::as_bytes(std::span(indices));
    auto     index_buffer = device->create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value()) << "Failed to create index buffer";

    // Render loop - keep window open for visual verification
    int frame_count = 0;
    while (!window->should_close())
    {
        // Poll window events to keep it responsive
        window->poll_events();

        // Draw the triangle
        auto draw_result = device->draw_indexed(vertex_buffer.value(), index_buffer.value(), 3);
        if (!draw_result.has_value())
        {
            FAIL() << "Failed to draw triangle on frame " << frame_count;
            break;
        }

        // Present to screen
        device->present();

        frame_count++;

        // Limit to ~60 FPS to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

TEST(VisualTest, DISABLED_SpinningCube)
{
    // Create window FIRST
    WindowConfig window_config;
    window_config.width     = 800;
    window_config.height    = 600;
    window_config.title     = "WebGPU Spinning Cube Test";
    window_config.resizable = false;

    auto window_result = create_window(window_config);
    ASSERT_TRUE(window_result.has_value()) << "Failed to create window";

    auto& window = window_result.value();

    // Create render context with the window (caller owns window)
    RenderConfig render_config;
    render_config.backend           = BackendType::WebGPU;
    render_config.enable_validation = false;

    auto render_context = create_render_context();
    ASSERT_NE(render_context, nullptr);

    auto render_ctx_result = render_context->initialize(render_config, window.get());
    ASSERT_TRUE(render_ctx_result.has_value()) << "Failed to initialize RenderContext with WebGPU backend";

    // Create WebGPU device
    auto device = render_context->device();
    ASSERT_NE(device, nullptr) << "Failed to get WebGPU device";

    device->set_aspect_ratio(raktr::render::AspectRatio::Ratio_16_9);
    window->set_resize_callback([&](uint32_t width, uint32_t height)
                                {
                                    [[maybe_unused]] auto resize_result = device->resize(width, height);
                                });

    // Create cube vertex buffer - 8 vertices at corners (scaled down to 0.5 units)
    // clang-format off
    float vertices[] = {
        // Back face (z = -0.5)
        -0.5f, -0.5f, -0.5f,  // 0: back-bottom-left
         0.5f, -0.5f, -0.5f,  // 1: back-bottom-right
         0.5f,  0.5f, -0.5f,  // 2: back-top-right
        -0.5f,  0.5f, -0.5f,  // 3: back-top-left
        // Front face (z = +0.5)
        -0.5f, -0.5f,  0.5f,  // 4: front-bottom-left
         0.5f, -0.5f,  0.5f,  // 5: front-bottom-right
         0.5f,  0.5f,  0.5f,  // 6: front-top-right
        -0.5f,  0.5f,  0.5f   // 7: front-top-left
    };
    // clang-format on

    auto vertex_data   = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device->create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value()) << "Failed to create vertex buffer";

    // Create index buffer - 36 indices for 12 triangles (2 per face, 6 faces)
    // All triangles wound counter-clockwise when viewed from outside
    // clang-format off
    uint32_t indices[] = {
        // Back face (looking at -Z)
        0, 2, 1,  0, 3, 2,
        // Front face (looking at +Z)
        4, 5, 6,  4, 6, 7,
        // Left face (looking at -X)
        4, 7, 3,  4, 3, 0,
        // Right face (looking at +X)
        1, 2, 6,  1, 6, 5,
        // Bottom face (looking at -Y)
        0, 1, 5,  0, 5, 4,
        // Top face (looking at +Y)
        3, 6, 2,  3, 7, 6
    };
    // clang-format on
    auto index_data   = std::as_bytes(std::span(indices));
    auto index_buffer = device->create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value()) << "Failed to create index buffer";

    // Create uniform buffer for MVP matrix
    auto uniform_buffer = device->create_uniform_buffer(16 * sizeof(float));
    ASSERT_TRUE(uniform_buffer.has_value()) << "Failed to create uniform buffer";

    // Initialize with identity matrix
    // clang-format off
    float initial_mvp[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    // clang-format on
    auto initial_data = std::as_bytes(std::span(initial_mvp));
    auto init_result  = device->update_uniform_buffer(uniform_buffer.value(), initial_data);
    ASSERT_TRUE(init_result.has_value()) << "Failed to initialize uniform buffer";

    // Set the uniform buffer once (no need to set it every frame)
    device->set_uniform_buffer(uniform_buffer.value());

    // Animation state
    float       angle          = 0.0f;
    const float rotation_speed = 1.0f; // radians per second

    // Render loop
    int frame_count = 0;
    while (!window->should_close())
    {
        // Create transformation matrices using GLM
        auto model = math::create_rotation(angle, glm::vec3(0.0f, 1.0f, 0.0f));
        auto view  = math::create_look_at(
            glm::vec3(0.0f, 0.0f, 3.0f), // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f), // Look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
        );
        auto projection = math::create_perspective(
            glm::radians(45.0f),                                            // FOV
            static_cast<float>(window_config.width) / window_config.height, // Aspect ratio
            0.1f,                                                           // Near plane
            100.0f                                                          // Far plane
        );

        // Compute MVP = projection * view * model
        glm::mat4 mvp = projection * view * model;

        // Update angle for next frame (assuming ~60 FPS, so ~0.0167 seconds per frame)
        angle += rotation_speed * 0.016f;

        // Update uniform buffer with new MVP using GLM helper
        auto update_result = device->update_uniform_buffer(
            uniform_buffer.value(),
            math::matrix_to_bytes(mvp));
        ASSERT_TRUE(update_result.has_value()) << "Failed to update uniform buffer";

        // Rebind the uniform buffer to ensure it sees the updated data
        device->set_uniform_buffer(uniform_buffer.value());

        // Poll window events
        window->poll_events();

        // Draw the cube
        auto draw_result = device->draw_indexed(vertex_buffer.value(), index_buffer.value(), 36);
        if (!draw_result.has_value())
        {
            FAIL() << "Failed to draw cube on frame " << frame_count;
            break;
        }

        // Present to screen
        device->present();

        frame_count++;

        // Limit to ~60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

TEST(VisualTest, DISABLED_SpinningCubeTypeSafe)
{
    // Create window FIRST
    WindowConfig window_config;
    window_config.width      = 1920;
    window_config.height     = 1080;
    window_config.title      = "WebGPU Spinning Cube (Type-Safe) Test - WASD + Mouse to control camera, ESC to exit";
    window_config.resizable  = true;
    window_config.fullscreen = false;

    auto window_result = create_window(window_config);
    ASSERT_TRUE(window_result.has_value()) << "Failed to create window";

    auto& window = window_result.value();

    // Create render context with the window (caller owns window)
    RenderConfig render_config;
    render_config.backend           = BackendType::WebGPU;
    render_config.enable_validation = false;

    auto render_context = create_render_context();
    ASSERT_NE(render_context, nullptr);

    auto render_ctx_result = render_context->initialize(render_config, window.get());
    ASSERT_TRUE(render_ctx_result.has_value()) << "Failed to initialize RenderContext with WebGPU backend";

    // Create WebGPU device
    auto device = render_context->device();
    ASSERT_NE(device, nullptr) << "Failed to get WebGPU device";

    device->set_aspect_ratio(raktr::render::AspectRatio::Ratio_16_9);
    window->set_resize_callback([&](uint32_t width, uint32_t height)
                                {
                                    [[maybe_unused]] auto resize_result = device->resize(width, height);
                                });

    // Create input system
    raktr::engine::InputSystem input_system(*window);

    // Create camera
    raktr::engine::scene::Camera camera(
        glm::vec3(0.0F, 0.0F, 5.0F), // Start 5 units back from origin
        45.0F,                       // 45° FOV
        static_cast<float>(window_config.width) / static_cast<float>(window_config.height),
        0.1F,  // Near plane
        100.0F // Far plane
    );
    camera.set_movement_speed(5.0F);
    camera.set_mouse_sensitivity(0.1F);

    // Create cube vertex buffer - 8 vertices at corners (scaled down to 0.5 units)
    // clang-format off
    float vertices[] = {
        // Back face (z = -0.5)
        -0.5f, -0.5f, -0.5f,  // 0: back-bottom-left
         0.5f, -0.5f, -0.5f,  // 1: back-bottom-right
         0.5f,  0.5f, -0.5f,  // 2: back-top-right
        -0.5f,  0.5f, -0.5f,  // 3: back-top-left
        // Front face (z = +0.5)
        -0.5f, -0.5f,  0.5f,  // 4: front-bottom-left
         0.5f, -0.5f,  0.5f,  // 5: front-bottom-right
         0.5f,  0.5f,  0.5f,  // 6: front-top-right
        -0.5f,  0.5f,  0.5f   // 7: front-top-left
    };
    // clang-format on

    auto vertex_data   = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device->create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value()) << "Failed to create vertex buffer";

    // Create index buffer - 36 indices for 12 triangles (2 per face, 6 faces)
    // clang-format off
    uint32_t indices[] = {
        // Back face (looking at -Z)
        0, 2, 1,  0, 3, 2,
        // Front face (looking at +Z)
        4, 5, 6,  4, 6, 7,
        // Left face (looking at -X)
        4, 7, 3,  4, 3, 0,
        // Right face (looking at +X)
        1, 2, 6,  1, 6, 5,
        // Bottom face (looking at -Y)
        0, 1, 5,  0, 5, 4,
        // Top face (looking at +Y)
        3, 6, 2,  3, 7, 6
    };
    // clang-format on
    auto index_data   = std::as_bytes(std::span(indices));
    auto index_buffer = device->create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value()) << "Failed to create index buffer";

    // Create uniform buffer for MVP matrix
    auto uniform_buffer = device->create_uniform_buffer(16 * sizeof(float));
    ASSERT_TRUE(uniform_buffer.has_value()) << "Failed to create uniform buffer";

    // Initialize with identity matrix
    math::ModelViewProjection identity(glm::mat4(1.0f));
    auto                      init_result = device->update_uniform_buffer(uniform_buffer.value(), identity.to_bytes());
    ASSERT_TRUE(init_result.has_value()) << "Failed to initialize uniform buffer";

    // Set the uniform buffer once (no need to set it every frame)
    device->set_uniform_buffer(uniform_buffer.value());

    // Animation state
    float       angle          = 0.0F;
    const float rotation_speed = 1.0F; // radians per second

    // Frame timing
    auto last_frame_time = std::chrono::high_resolution_clock::now();

    // Render loop
    int frame_count = 0;
    while (!window->should_close())
    {
        // Calculate delta time
        auto  current_frame_time = std::chrono::high_resolution_clock::now();
        float delta_time         = std::chrono::duration<float>(current_frame_time - last_frame_time).count();
        last_frame_time          = current_frame_time;

        // Poll window events (triggers callbacks)
        window->poll_events();

        // Process input and update camera
        auto input_state = input_system.process_events();
        camera.process_input(input_state, delta_time);

        // Check for ESC to exit
        if (input_state.keys[raktr::engine::KeyCode::Escape])
        {
            break;
        }

        // Create model transformation (spinning cube)
        math::Rotation model_rotation(angle, math::Axis::Y());

        // Get camera matrices
        math::View        view       = camera.view();
        math::Perspective projection = camera.projection();

        // Compose transformations with type-safe operators
        math::ModelViewProjection mvp = projection * view * model_rotation;

        // Update angle for next frame
        angle += rotation_speed * delta_time;

        // Update uniform buffer using type-safe to_bytes()
        auto update_result = device->update_uniform_buffer(
            uniform_buffer.value(),
            mvp.to_bytes());
        ASSERT_TRUE(update_result.has_value()) << "Failed to update uniform buffer";

        // Rebind the uniform buffer to ensure it sees the updated data
        device->set_uniform_buffer(uniform_buffer.value());

        // Draw the cube
        auto draw_result = device->draw_indexed(vertex_buffer.value(), index_buffer.value(), 36);
        if (!draw_result.has_value())
        {
            FAIL() << "Failed to draw cube on frame " << frame_count;
            break;
        }

        // Present to screen
        device->present();

        frame_count++;

        // Limit to ~60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
