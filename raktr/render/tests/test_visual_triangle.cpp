/*!
 * @file test_visual_triangle.cpp
 * @brief Visual test for triangle rendering - keeps window open for visual verification.
 * 
 * This is not an automated test. Run manually to see the rendered triangle.
 * Press ESC or close the window to exit.
 */

#include "backend/wgpu/wgpu_device.h"
#include "window/window.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

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
