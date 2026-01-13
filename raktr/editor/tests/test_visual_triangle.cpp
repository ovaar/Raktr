/*!
 * @file test_visual_triangle.cpp
 * @brief Visual test for triangle rendering - keeps window open for visual verification.
 *
 * This is not an automated test. Run manually to see the rendered triangle.
 * Press ESC or close the window to exit.
 */
#include "aspect_ratio.h"
#include "buffer.h"
#include "input/camera_controller.h" // From engine module
#include "input/input_system.h"      // From engine module
#include "instance_data.h"
#include "math/transform.h"
#include "math/transform_types.h"
#include "render_context.h"
#include "scene/camera.h" // From engine module
#include "window/window.h"

#include <algorithm>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <unordered_set>

// For frustum culling demo
#include "scene/octree.h"

// For render pass architecture
#include "backend/wgpu/passes/wgpu_geometry_pass.h"
#include "backend/wgpu/passes/wgpu_hi_z_occlusion_pass.h"
#include "backend/wgpu/passes/wgpu_hi_z_pyramid_pass.h"
#include "backend/wgpu/passes/wgpu_instanced_geometry_pass.h"
#include "backend/wgpu/wgpu_device.h"
#include "backend/wgpu/wgpu_pass_context.h"
#include "render_graph.h"
#include <webgpu/webgpu.h>

using namespace raktr::render;
using namespace raktr::render::backend::wgpu;

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

    // Set resize callback to update both device and camera
    window->set_resize_callback([&](uint32_t width, uint32_t height)
                                {
                                    camera.set_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));
                                    [[maybe_unused]] auto resize_result = device->resize(width, height);
                                });

    // Create camera controller with FPS controls (WASD + QE + mouse look)
    raktr::engine::input::CameraController camera_controller =
        raktr::engine::input::CameraController::fps_controller(5.0F, 0.1F);

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
        camera_controller.update(input_state, camera, delta_time);

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

TEST(VisualTest, DISABLED_FrustumCullingDemo)
{
    // Create window
    WindowConfig window_config;
    window_config.width      = 1920;
    window_config.height     = 1080;
    window_config.title      = "Frustum Culling Demo - WASD + Mouse to move, ESC to exit";
    window_config.resizable  = true;
    window_config.fullscreen = false;

    auto window_result = create_window(window_config);
    ASSERT_TRUE(window_result.has_value()) << "Failed to create window";
    auto& window = window_result.value();

    // Create render context
    RenderConfig render_config;
    render_config.backend           = BackendType::WebGPU;
    render_config.enable_validation = false;

    auto render_context = create_render_context();
    ASSERT_NE(render_context, nullptr);

    auto render_ctx_result = render_context->initialize(render_config, window.get());
    ASSERT_TRUE(render_ctx_result.has_value()) << "Failed to initialize RenderContext";

    auto device = render_context->device();
    ASSERT_NE(device, nullptr);
    device->set_aspect_ratio(raktr::render::AspectRatio::Ratio_16_9);

    // Create input system and camera
    raktr::engine::InputSystem   input_system(*window);
    raktr::engine::scene::Camera camera(
        glm::vec3(0.0F, 5.0F, 15.0F), // Start above and back
        45.0F,
        static_cast<float>(window_config.width) / static_cast<float>(window_config.height),
        0.1F,
        100.0F);

    window->set_resize_callback([&](uint32_t width, uint32_t height)
                                {
                                    camera.set_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));
                                    [[maybe_unused]] auto resize_result = device->resize(width, height);
                                });

    raktr::engine::input::CameraController camera_controller =
        raktr::engine::input::CameraController::fps_controller(10.0F, 0.1F);

    // Create cube geometry (same as spinning cube)
    float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f
    };
    auto vertex_buffer = device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vertex_buffer.has_value());

    uint32_t indices[] = {
        0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 4, 7, 3, 4, 3, 0, 1, 2, 6, 1, 6, 5, 0, 1, 5, 0, 5, 4, 3, 6, 2, 3, 7, 6
    };
    auto index_buffer = device->create_index_buffer(std::as_bytes(std::span(indices)));
    ASSERT_TRUE(index_buffer.has_value());

    auto uniform_buffer = device->create_uniform_buffer(16 * sizeof(float));
    ASSERT_TRUE(uniform_buffer.has_value());

    math::ModelViewProjection identity(glm::mat4(1.0f));
    auto                      init_result = device->update_uniform_buffer(uniform_buffer.value(), identity.to_bytes());
    ASSERT_TRUE(init_result.has_value());

    // Create octree and grid of cubes
    raktr::engine::scene::Octree octree(glm::vec3(0, 0, 0), 50.0f, 4);

    struct CubeInstance
    {
        raktr::engine::scene::Octree::ObjectId id;
        glm::vec3                              position;
        float                                  rotation_speed;
    };

    std::vector<CubeInstance>              cubes;
    raktr::engine::scene::Octree::ObjectId next_id = 1;

    // Create 10x5x10 grid of cubes (500 cubes)
    for (int x = -5; x < 5; ++x)
    {
        for (int y = 0; y < 5; ++y)
        {
            for (int z = -5; z < 5; ++z)
            {
                glm::vec3 pos(x * 3.0f, y * 3.0f, z * 3.0f);
                float     rotation_speed = 0.5f + (rand() % 100) / 100.0f;

                cubes.push_back({ next_id, pos, rotation_speed });
                octree.insert(next_id, pos, 0.7f); // Slightly larger than 0.5 cube half-size
                next_id++;
            }
        }
    }

    spdlog::info("Created {} cubes in octree", cubes.size());

    // Create instance buffer (will be updated each frame with visible cubes)
    std::vector<InstanceData> instance_data;
    instance_data.reserve(cubes.size());

    // Initialize with all cubes (will be filtered by frustum culling)
    for (const auto& cube : cubes)
    {
        InstanceData inst;
        inst.model_matrix = glm::translate(glm::mat4(1.0f), cube.position);
        inst.color        = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // White
        instance_data.push_back(inst);
    }

    auto instance_buffer_result = device->create_instance_buffer(
        std::as_bytes(std::span(instance_data)));
    ASSERT_TRUE(instance_buffer_result.has_value()) << "Failed to create instance buffer";
    auto instance_buffer = instance_buffer_result.value();

    // Frame timing
    auto  last_frame_time = std::chrono::high_resolution_clock::now();
    float total_time      = 0.0f;

    // Render loop
    while (!window->should_close())
    {
        auto  current_frame_time = std::chrono::high_resolution_clock::now();
        float delta_time         = std::chrono::duration<float>(current_frame_time - last_frame_time).count();
        last_frame_time          = current_frame_time;
        total_time += delta_time;

        window->poll_events();

        auto input_state = input_system.process_events();
        camera_controller.update(input_state, camera, delta_time);

        if (input_state.keys[raktr::engine::KeyCode::Escape])
        {
            break;
        }

        // Query visible cubes using frustum culling
        auto frustum     = camera.frustum();
        auto visible_ids = octree.query_frustum(frustum);

        // Create set for fast lookup
        std::unordered_set<raktr::engine::scene::Octree::ObjectId> visible_set(
            visible_ids.begin(), visible_ids.end());

        // Build instance data for visible cubes only
        instance_data.clear();
        for (const auto& cube : cubes)
        {
            if (visible_set.count(cube.id) > 0)
            {
                InstanceData inst;
                // Apply per-instance rotation based on time and speed
                float     angle    = total_time * cube.rotation_speed;
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
                inst.model_matrix  = glm::translate(glm::mat4(1.0f), cube.position) * rotation;

                // Color based on Y position for visual variety
                float t    = (cube.position.y + 7.5f) / 15.0f; // Normalize to 0-1
                inst.color = glm::vec4(0.3f + t * 0.7f, 0.5f, 1.0f - t * 0.5f, 1.0f);
                instance_data.push_back(inst);
            }
        }

        size_t visible_count = instance_data.size();

        // Get camera matrices (View-Projection only, model is per-instance)
        math::View        view       = camera.view();
        math::Perspective projection = camera.projection();

        // Update uniform with VP matrix (not MVP, since model is per-instance)
        math::ModelViewProjection vp(projection.matrix() * view.matrix());
        auto                      update_result = device->update_uniform_buffer(uniform_buffer.value(), vp.to_bytes());
        ASSERT_TRUE(update_result.has_value());

        device->set_uniform_buffer(uniform_buffer.value());

        // Update instance buffer with visible cubes
        if (!instance_data.empty())
        {
            auto update_inst_result = device->update_instance_buffer(
                instance_buffer,
                std::as_bytes(std::span(instance_data)));
            ASSERT_TRUE(update_inst_result.has_value());

            // Draw all visible cubes with one instanced draw call
            auto draw_result = device->draw_indexed_instanced(
                vertex_buffer.value(),
                index_buffer.value(),
                instance_buffer,
                36,                                           // index count
                static_cast<uint32_t>(instance_data.size())); // instance count

            if (!draw_result.has_value())
            {
                FAIL() << "Failed to draw instanced cubes";
                break;
            }
        }

        device->present();

        // Log culling stats every second
        static auto                  last_log_time = std::chrono::high_resolution_clock::now();
        auto                         now           = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed       = now - last_log_time;

        if (elapsed.count() >= 1.0f)
        {
            spdlog::info("Frustum Culling: Visible {} / Total {} (Culled {}) - Camera pos: ({:.1f}, {:.1f}, {:.1f})",
                         visible_count,
                         cubes.size(),
                         cubes.size() - visible_count,
                         camera.position().x,
                         camera.position().y,
                         camera.position().z);
            last_log_time = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

TEST(VisualTest, DISABLED_OcclusionCullingDemo)
{
    // Create window
    WindowConfig window_config;
    window_config.width      = 1920;
    window_config.height     = 1080;
    window_config.title      = "Temporal Hi-Z Occlusion Culling Demo - WASD+QE+Mouse, O to toggle, ESC to exit";
    window_config.resizable  = true;
    window_config.fullscreen = false;

    auto window_result = create_window(window_config);
    ASSERT_TRUE(window_result.has_value()) << "Failed to create window";
    auto& window = window_result.value();

    // Create render context
    RenderConfig render_config;
    render_config.backend           = BackendType::WebGPU;
    render_config.enable_validation = false;

    auto render_context = create_render_context();
    ASSERT_NE(render_context, nullptr);

    auto render_ctx_result = render_context->initialize(render_config, window.get());
    ASSERT_TRUE(render_ctx_result.has_value()) << "Failed to initialize RenderContext";

    auto device = render_context->device();
    ASSERT_NE(device, nullptr);
    device->set_aspect_ratio(raktr::render::AspectRatio::Ratio_16_9);

    // Create input system and camera
    raktr::engine::InputSystem   input_system(*window);
    raktr::engine::scene::Camera camera(
        glm::vec3(0.0F, 5.0F, 30.0F), // Start further back to see the scene
        45.0F,
        static_cast<float>(window_config.width) / static_cast<float>(window_config.height),
        0.1F,
        200.0F);

    window->set_resize_callback([&](uint32_t width, uint32_t height)
                                {
                                    camera.set_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));
                                    [[maybe_unused]] auto resize_result = device->resize(width, height);
                                });

    raktr::engine::input::CameraController camera_controller =
        raktr::engine::input::CameraController::fps_controller(15.0F, 0.1F);

    // Create cube geometry
    float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f
    };
    auto vertex_buffer = device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vertex_buffer.has_value());

    uint32_t indices[] = {
        0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 4, 7, 3, 4, 3, 0, 1, 2, 6, 1, 6, 5, 0, 1, 5, 0, 5, 4, 3, 6, 2, 3, 7, 6
    };
    auto index_buffer = device->create_index_buffer(std::as_bytes(std::span(indices)));
    ASSERT_TRUE(index_buffer.has_value());

    auto uniform_buffer = device->create_uniform_buffer(16 * sizeof(float));
    ASSERT_TRUE(uniform_buffer.has_value());

    math::ModelViewProjection identity(glm::mat4(1.0f));
    auto                      init_result = device->update_uniform_buffer(uniform_buffer.value(), identity.to_bytes());
    ASSERT_TRUE(init_result.has_value());

    // Bind uniform buffer to device's render pipeline
    device->set_uniform_buffer(uniform_buffer.value());

    // Create scene: Large occluders (walls) and small objects behind them
    struct SceneObject
    {
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec4 color;
        bool      is_occluder; // Large walls that block visibility
        bool      is_occluded; // Objects that should be occluded
    };

    std::vector<SceneObject> scene_objects;

    // Central occluder wall (large, at z = 0)
    scene_objects.push_back({
        glm::vec3(0.0f, 5.0f, 0.0f),       // position
        glm::vec3(15.0f, 10.0f, 1.0f),     // scale (wide and tall wall)
        glm::vec4(0.8f, 0.2f, 0.2f, 1.0f), // red
        true,                              // is_occluder
        false                              // not occluded
    });

    // Left occluder wall
    scene_objects.push_back({ glm::vec3(-20.0f, 5.0f, -10.0f),
                              glm::vec3(1.0f, 10.0f, 15.0f),
                              glm::vec4(0.2f, 0.8f, 0.2f, 1.0f), // green
                              true,
                              false });

    // Right occluder wall
    scene_objects.push_back({ glm::vec3(20.0f, 5.0f, -10.0f),
                              glm::vec3(1.0f, 10.0f, 15.0f),
                              glm::vec4(0.2f, 0.2f, 0.8f, 1.0f), // blue
                              true,
                              false });

    // Objects behind the central wall (should be occluded)
    for (int x = -3; x <= 3; ++x)
    {
        for (int y = 0; y < 5; ++y)
        {
            scene_objects.push_back({
                glm::vec3(x * 3.0f, y * 2.0f + 1.0f, -10.0f), // behind wall
                glm::vec3(0.8f, 0.8f, 0.8f),
                glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), // yellow
                false,                             // not occluder
                true                               // should be occluded
            });
        }
    }

    // Objects behind left wall
    for (int z = -5; z < 5; ++z)
    {
        for (int y = 0; y < 3; ++y)
        {
            scene_objects.push_back({ glm::vec3(-25.0f, y * 3.0f + 1.0f, z * 2.0f),
                                      glm::vec3(0.8f, 0.8f, 0.8f),
                                      glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), // orange
                                      false,
                                      true });
        }
    }

    // Objects behind right wall
    for (int z = -5; z < 5; ++z)
    {
        for (int y = 0; y < 3; ++y)
        {
            scene_objects.push_back({ glm::vec3(25.0f, y * 3.0f + 1.0f, z * 2.0f),
                                      glm::vec3(0.8f, 0.8f, 0.8f),
                                      glm::vec4(0.5f, 0.0f, 1.0f, 1.0f), // purple
                                      false,
                                      true });
        }
    }

    // Some visible objects in front
    for (int x = -2; x <= 2; ++x)
    {
        scene_objects.push_back({ glm::vec3(x * 5.0f, 2.0f, 15.0f), // in front
                                  glm::vec3(1.0f, 1.0f, 1.0f),
                                  glm::vec4(0.0f, 1.0f, 1.0f, 1.0f), // cyan
                                  false,
                                  false });
    }

    spdlog::info("Created {} objects in scene (3 occluders, {} potentially occluded)",
                 scene_objects.size(),
                 std::count_if(scene_objects.begin(), scene_objects.end(), [](const SceneObject& obj)
                               {
                                   return obj.is_occluded;
                               }));

    // Create instance buffer with initial data
    std::vector<InstanceData> instance_data;
    instance_data.reserve(scene_objects.size());

    // Initialize with all objects visible
    for (const auto& obj : scene_objects)
    {
        InstanceData inst;
        inst.model_matrix = glm::translate(glm::mat4(1.0f), obj.position) *
                            glm::scale(glm::mat4(1.0f), obj.scale);
        inst.color = obj.color;
        instance_data.push_back(inst);
    }

    auto instance_buffer_result = device->create_instance_buffer(
        std::as_bytes(std::span(instance_data)));
    ASSERT_TRUE(instance_buffer_result.has_value()) << "Failed to create instance buffer";
    // auto instance_buffer = instance_buffer_result.value();

    // Create Hi-Z buffer for temporal GPU occlusion culling
    std::unique_ptr<raktr::render::occlusion::HiZBuffer> hi_z_buffer;

    try
    {
        auto occlusion_ops = device->capability<raktr::render::capabilities::OcclusionCullingOps>();
        auto hi_z_result   = occlusion_ops.create_hi_z_buffer(window_config.width, window_config.height);
        if (hi_z_result)
        {
            hi_z_buffer = std::move(hi_z_result.value());
            spdlog::info("Hi-Z buffer initialized: {}x{} with {} mip levels",
                         hi_z_buffer->width(),
                         hi_z_buffer->height(),
                         hi_z_buffer->mip_levels());
        }
        else
        {
            spdlog::warn("Failed to create Hi-Z buffer, GPU occlusion culling disabled");
        }
    }
    catch (const std::exception& e)
    {
        spdlog::warn("Device does not support OcclusionCullingOps capability: {}", e.what());
    }

    // Prepare AABBs for all scene objects (non-occluders only - occluders always render)
    std::vector<raktr::render::occlusion::AABB> scene_aabbs;
    std::vector<size_t>                         aabb_to_object_index; // Map AABB index to scene object index
    scene_aabbs.reserve(scene_objects.size());
    aabb_to_object_index.reserve(scene_objects.size());

    for (size_t i = 0; i < scene_objects.size(); ++i)
    {
        const auto& obj = scene_objects[i];
        if (!obj.is_occluder) // Only test non-occluders
        {
            glm::vec3 half_extent = obj.scale * 0.5f;
            scene_aabbs.push_back({
                obj.position - half_extent, // min
                obj.position + half_extent  // max
            });
            aabb_to_object_index.push_back(i);
        }
    }

    spdlog::info("Testing {} objects for occlusion (occluders always visible)", scene_aabbs.size());

    // Visibility results for non-occluder objects
    std::vector<bool> visibility_results;
    visibility_results.reserve(scene_aabbs.size());

    // Occlusion culling toggle
    bool occlusion_culling_enabled = true; // Start with culling ENABLED to see the effect

    // Build complete instance data for all scene objects
    // This will be used by WgpuInstancedGeometryPass
    std::vector<InstanceData> all_instance_data;
    all_instance_data.reserve(scene_objects.size());
    for (const auto& obj : scene_objects)
    {
        InstanceData inst;
        inst.model_matrix = glm::translate(glm::mat4(1.0f), obj.position) *
                            glm::scale(glm::mat4(1.0f), obj.scale);
        inst.color = obj.color;
        all_instance_data.push_back(inst);
    }

    spdlog::info("=== Temporal Hi-Z Occlusion Culling Architecture ===");
    spdlog::info("RenderGraph with 2 passes:");
    spdlog::info("  Pass 1: WgpuHiZOcclusionPass       → Test visibility against previous frame's pyramid");
    spdlog::info("  Pass 2: WgpuInstancedGeometryPass  → Render visible objects using GPU instancing");
    spdlog::info("  [Pass 3: WgpuHiZPyramidPass would build pyramid - done manually for demo]");
    spdlog::info("");
    spdlog::info("Press 'O' to toggle occlusion culling on/off");

    // Frame timing
    auto     last_frame_time = std::chrono::high_resolution_clock::now();
    bool     last_o_pressed  = false;
    uint64_t frame_index     = 0;

    // Render loop
    while (!window->should_close())
    {
        auto  current_frame_time = std::chrono::high_resolution_clock::now();
        float delta_time         = std::chrono::duration<float>(current_frame_time - last_frame_time).count();
        last_frame_time          = current_frame_time;

        window->poll_events();

        auto input_state = input_system.process_events();
        camera_controller.update(input_state, camera, delta_time);

        if (input_state.keys[raktr::engine::KeyCode::Escape])
        {
            break;
        }

        // Toggle occlusion culling with O key
        bool o_pressed = input_state.keys[raktr::engine::KeyCode::O];
        if (o_pressed && !last_o_pressed)
        {
            occlusion_culling_enabled = !occlusion_culling_enabled;
            spdlog::info("Occlusion Culling: {} (Frame {})",
                         occlusion_culling_enabled ? "ENABLED" : "DISABLED",
                         frame_index);
        }
        last_o_pressed = o_pressed;

        // Get camera matrices
        math::View        view            = camera.view();
        math::Perspective projection      = camera.projection();
        glm::mat4         view_projection = projection.matrix() * view.matrix();

        // Update uniform buffer with view-projection matrix using Queue
        math::ModelViewProjection vp(view_projection);
        device->queue().write_buffer(uniform_buffer.value(), 0, vp.to_bytes());

        // ========================================================================
        // RenderGraph with Hi-Z Occlusion Culling + Geometry Passes
        // ========================================================================

        // // Rebuild RenderGraph each frame (lightweight - just allocates wrappers)
        // RenderGraph frame_graph(device);

        // // Pass 1: Hi-Z Occlusion Pass - Test visibility using previous frame's pyramid
        // if (occlusion_culling_enabled && hi_z_buffer)
        // {
        //     frame_graph.add_pass(WgpuHiZOcclusionPass{
        //         hi_z_buffer.get(),
        //         &scene_aabbs,
        //         view_projection,
        //         &visibility_results });

        //     // Execute occlusion pass first to populate visibility_results
        //     WgpuPassContext occlusion_ctx{};
        //     occlusion_ctx.frame_index     = frame_index;
        //     occlusion_ctx.command_encoder = nullptr;
        //     occlusion_ctx.color_target    = nullptr;
        //     occlusion_ctx.depth_target    = nullptr;
        //     occlusion_ctx.viewport_width  = window_config.width;
        //     occlusion_ctx.viewport_height = window_config.height;

        //     frame_graph.execute(occlusion_ctx);
        // }
        // else
        // {
        //     // Fallback: mark all objects visible
        //     visibility_results.assign(scene_aabbs.size(), true);
        // }

        // // Map visibility_results (per-AABB) back to per-instance visibility (per scene object)
        // std::vector<bool> per_instance_visibility(scene_objects.size(), false);

        // // Occluders always visible (first 3 objects)
        // for (size_t i = 0; i < 3 && i < scene_objects.size(); ++i)
        // {
        //     if (scene_objects[i].is_occluder)
        //     {
        //         per_instance_visibility[i] = true;
        //     }
        // }

        // // Map AABB visibility back to scene object indices
        // size_t aabb_index = 0;
        // for (size_t i = 0; i < scene_objects.size(); ++i)
        // {
        //     if (!scene_objects[i].is_occluder)
        //     {
        //         if (aabb_index < visibility_results.size())
        //         {
        //             per_instance_visibility[i] = visibility_results[aabb_index];
        //         }
        //         aabb_index++;
        //     }
        // }

        // // Pass 2: WgpuInstancedGeometryPass - Render visible objects using GPU instancing
        // // Create new render graph for geometry pass
        // RenderGraph geometry_graph(device);
        // geometry_graph.add_pass(WgpuInstancedGeometryPass{
        //     device,
        //     vertex_buffer.value(),
        //     index_buffer.value(),
        //     instance_buffer,
        //     &all_instance_data,
        //     &per_instance_visibility,
        //     36 // index count per cube
        // });

        // // Execute geometry rendering pass
        // WgpuPassContext geometry_ctx{};
        // geometry_ctx.frame_index     = frame_index;
        // geometry_ctx.command_encoder = nullptr;
        // geometry_ctx.color_target    = nullptr;
        // geometry_ctx.depth_target    = nullptr;
        // geometry_ctx.viewport_width  = window_config.width;
        // geometry_ctx.viewport_height = window_config.height;

        // geometry_graph.execute(geometry_ctx);

        // // ========================================================================
        // // Pass 3: Hi-Z Pyramid Pass - Build pyramid for NEXT frame
        // // ========================================================================
        // // Note: This must happen AFTER geometry rendering so depth buffer is populated
        // if (occlusion_culling_enabled && hi_z_buffer)
        // {
        //     // Create separate graph for pyramid building (after present)
        //     RenderGraph pyramid_graph(device);
        //     pyramid_graph.add_pass(WgpuHiZPyramidPass{
        //         hi_z_buffer.get(),
        //         static_cast<WGPUTexture>(device->get_depth_texture()) });

        //     WgpuPassContext pyramid_ctx{};
        //     pyramid_ctx.frame_index     = frame_index;
        //     pyramid_ctx.command_encoder = nullptr;
        //     pyramid_ctx.color_target    = nullptr;
        //     pyramid_ctx.depth_target    = static_cast<WGPUTextureView>(device->get_depth_view());
        //     pyramid_ctx.viewport_width  = window_config.width;
        //     pyramid_ctx.viewport_height = window_config.height;

        //     pyramid_graph.execute(pyramid_ctx);
        // }

        // // Present (if rendering to swapchain)
        // device->present();

        frame_index++;
    }

    spdlog::info("Occlusion Culling Demo finished after {} frames", frame_index);
}
