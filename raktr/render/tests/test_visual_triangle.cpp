/*!
 * @file test_visual_triangle.cpp
 * @brief Visual test for triangle rendering - keeps window open for visual verification.
 * 
 * This is not an automated test. Run manually to see the rendered triangle.
 * Press ESC or close the window to exit.
 */

#include "backend/wgpu/wgpu_device.h"
#include "window/window.h"
#include "math/transform.h"
#include <gtest/gtest.h>
#include <thread>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace raktr::render;
using namespace raktr::render::backend;

TEST(VisualTest, DISABLED_ManualRenderTriangle)
{
    // Create window
    WindowConfig config;
    config.width = 800;
    config.height = 600;
    config.title = "WebGPU Triangle Test";
    config.resizable = true;

    auto window_result = create_window(config);
    ASSERT_TRUE(window_result.has_value()) << "Failed to create window";
    
    auto& window = window_result.value();

    // Create WebGPU device
    auto device_result = WgpuDevice::create(window.get(), false);
    ASSERT_TRUE(device_result.has_value()) << "Failed to create WebGPU device";
    
    auto& device = device_result.value();

    // Create triangle vertex buffer
    // Positions in NDC space: top center, bottom left, bottom right
    float vertices[] = {
         0.0f,  0.6f, 0.0f,  // Top (will be greenish due to shader)
        -0.6f, -0.6f, 0.0f,  // Bottom-left (will be bluish)
         0.6f, -0.6f, 0.0f   // Bottom-right (will be reddish)
    };
    
    auto vertex_data = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device->create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value()) << "Failed to create vertex buffer";

    // Create index buffer
    uint32_t indices[] = {0, 1, 2};
    auto index_data = std::as_bytes(std::span(indices));
    auto index_buffer = device->create_index_buffer(index_data);
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
    // Create window
    WindowConfig config;
    config.width = 800;
    config.height = 600;
    config.title = "WebGPU Spinning Cube Test";
    config.resizable = false;

    auto window_result = create_window(config);
    ASSERT_TRUE(window_result.has_value()) << "Failed to create window";
    
    auto& window = window_result.value();

    // Create WebGPU device
    auto device_result = WgpuDevice::create(window.get(), false);
    ASSERT_TRUE(device_result.has_value()) << "Failed to create WebGPU device";
    
    auto& device = device_result.value();

    // Create cube vertex buffer - 8 vertices at corners (scaled down to 0.5 units)
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
    
    auto vertex_data = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device->create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value()) << "Failed to create vertex buffer";

    // Create index buffer - 36 indices for 12 triangles (2 per face, 6 faces)
    // All triangles wound counter-clockwise when viewed from outside
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
    auto index_data = std::as_bytes(std::span(indices));
    auto index_buffer = device->create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value()) << "Failed to create index buffer";

    // Create uniform buffer for MVP matrix
    auto uniform_buffer = device->create_uniform_buffer(16 * sizeof(float));
    ASSERT_TRUE(uniform_buffer.has_value()) << "Failed to create uniform buffer";

    // Initialize with identity matrix
    float initial_mvp[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    auto initial_data = std::as_bytes(std::span(initial_mvp));
    auto init_result = device->update_uniform_buffer(uniform_buffer.value(), initial_data);
    ASSERT_TRUE(init_result.has_value()) << "Failed to initialize uniform buffer";

    // Set the uniform buffer once (no need to set it every frame)
    device->set_uniform_buffer(uniform_buffer.value());

    // Animation state
    float angle = 0.0f;
    const float rotation_speed = 1.0f;  // radians per second

    // Render loop
    int frame_count = 0;
    while (!window->should_close())
    {
        // Create transformation matrices using GLM
        auto model = math::create_rotation(angle, glm::vec3(0.0f, 1.0f, 0.0f));
        auto view = math::create_look_at(
            glm::vec3(0.0f, 0.0f, 3.0f),  // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
        );
        auto projection = math::create_perspective(
            glm::radians(45.0f),                              // FOV
            static_cast<float>(config.width) / config.height, // Aspect ratio
            0.1f,                                             // Near plane
            100.0f                                            // Far plane
        );

        // Compute MVP = projection * view * model
        glm::mat4 mvp = projection * view * model;
        
        // Update angle for next frame (assuming ~60 FPS, so ~0.0167 seconds per frame)
        angle += rotation_speed * 0.016f;

        // Update uniform buffer with new MVP using GLM helper
        auto update_result = device->update_uniform_buffer(
            uniform_buffer.value(), 
            math::matrix_to_bytes(mvp)
        );
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
