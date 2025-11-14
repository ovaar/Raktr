/*!
 * @file test_camera_controller.cpp
 * @brief Unit tests for CameraController class.
 */

#include "input/camera_controller.h"
#include "input/input_system.h"
#include "scene/camera.h"
#include <glm/glm.hpp>
#include <gtest/gtest.h>

using namespace raktr::engine::scene;
using namespace raktr::engine;
using raktr::engine::input::CameraController;

namespace
{
    constexpr float EPSILON = 0.0001f;

    bool vec3_near(const glm::vec3& a, const glm::vec3& b, float epsilon = EPSILON)
    {
        return std::abs(a.x - b.x) < epsilon &&
               std::abs(a.y - b.y) < epsilon &&
               std::abs(a.z - b.z) < epsilon;
    }
} // namespace

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST(CameraController_Construction, DefaultConstructor_CreatesController)
{
    CameraController controller;
    SUCCEED(); // Just verify it constructs
}

TEST(CameraController_Presets, FPSController_HasDefaultBindings)
{
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // Test W key moves forward
    InputState input{};
    input.keys.set(KeyCode::W, true);
    controller.update(input, camera, 0.1f);

    glm::vec3 new_pos = camera.position();
    EXPECT_TRUE(vec3_near(new_pos, glm::vec3(0.0f, 0.0f, -1.0f), 0.01f));
}

TEST(CameraController_Presets, FlightController_HasDefaultBindings)
{
    CameraController controller = CameraController::flight_controller(10.0f, 0.1f);
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // Test W key moves forward
    InputState input{};
    input.keys.set(KeyCode::W, true);
    controller.update(input, camera, 0.1f);

    glm::vec3 new_pos = camera.position();
    EXPECT_TRUE(vec3_near(new_pos, glm::vec3(0.0f, 0.0f, -1.0f), 0.01f));
}

// ============================================================================
// Custom Binding Tests
// ============================================================================

TEST(CameraController_Binding, CustomBinding_ExecutesAction)
{
    CameraController controller;
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // Bind space key to move up by 5 units
    controller.bind_key(KeyCode::Space, [](Camera& cam, float dt)
                        {
                            glm::vec3 pos = cam.position();
                            cam.set_position(pos + glm::vec3(0, 5.0f, 0) * dt);
                        });

    InputState input{};
    input.keys.set(KeyCode::Space, true);
    controller.update(input, camera, 0.1f);

    glm::vec3 new_pos = camera.position();
    EXPECT_TRUE(vec3_near(new_pos, glm::vec3(0.0f, 0.5f, 0.0f), 0.01f));
}

TEST(CameraController_Binding, UnbindKey_RemovesBinding)
{
    CameraController controller;
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // Bind and then unbind
    controller.bind_key(KeyCode::W, [](Camera& cam, float dt)
                        {
                            glm::vec3 pos = cam.position();
                            cam.set_position(pos + glm::vec3(0, 1, 0) * dt);
                        });

    controller.unbind_key(KeyCode::W);

    InputState input{};
    input.keys.set(KeyCode::W, true);
    controller.update(input, camera, 0.1f);

    // Position should not have changed
    EXPECT_TRUE(vec3_near(camera.position(), glm::vec3(0, 0, 0)));
}

TEST(CameraController_Binding, ClearBindings_RemovesAllBindings)
{
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    controller.clear_bindings();

    InputState input{};
    input.keys.set(KeyCode::W, true);
    controller.update(input, camera, 0.1f);

    // Position should not have changed
    EXPECT_TRUE(vec3_near(camera.position(), glm::vec3(0, 0, 0)));
}

TEST(CameraController_Binding, RebindKey_ReplacesAction)
{
    CameraController controller;
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // Initial binding
    controller.bind_key(KeyCode::W, [](Camera& cam, float dt)
                        {
                            glm::vec3 pos = cam.position();
                            cam.set_position(pos + glm::vec3(0, 1, 0) * dt);
                        });

    // Rebind to different action
    controller.bind_key(KeyCode::W, [](Camera& cam, float dt)
                        {
                            glm::vec3 pos = cam.position();
                            cam.set_position(pos + glm::vec3(1, 0, 0) * dt);
                        });

    InputState input{};
    input.keys.set(KeyCode::W, true);
    controller.update(input, camera, 1.0f);

    // Should execute new action (move right, not up)
    EXPECT_TRUE(vec3_near(camera.position(), glm::vec3(1.0f, 0.0f, 0.0f), 0.01f));
}

// ============================================================================
// Mouse Look Tests
// ============================================================================

TEST(CameraController_MouseLook, MouseMovement_UpdatesRotation)
{
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // First frame to establish baseline
    InputState input{};
    input.mouse_x = 100.0;
    input.mouse_y = 0.0;
    controller.update(input, camera, 0.016f);

    // Second frame with mouse movement
    input.mouse_x = 200.0; // Moved 100 pixels right
    controller.update(input, camera, 0.016f);

    // Yaw should have increased (looking right)
    EXPECT_GT(camera.yaw(), -90.0f);
}

TEST(CameraController_MouseLook, DisableMouseLook_NoRotation)
{
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    controller.enable_mouse_look(false);

    InputState input{};
    input.mouse_x = 100.0;
    input.mouse_y = 0.0;
    controller.update(input, camera, 0.016f);

    input.mouse_x = 200.0;
    controller.update(input, camera, 0.016f);

    // Yaw should not have changed
    EXPECT_FLOAT_EQ(camera.yaw(), -90.0f);
}

TEST(CameraController_MouseLook, MouseSensitivity_AffectsRotationSpeed)
{
    CameraController controller1 = CameraController::fps_controller(10.0f, 0.1f);
    CameraController controller2 = CameraController::fps_controller(10.0f, 0.5f);

    Camera camera1(glm::vec3(0, 0, 0), 45.0f, 1.0f);
    Camera camera2(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // Same mouse movement for both
    InputState input1{};
    input1.mouse_x = 0.0;
    input1.mouse_y = 0.0;

    InputState input2 = input1;

    controller1.update(input1, camera1, 0.016f);
    controller2.update(input2, camera2, 0.016f);

    input1.mouse_x = 100.0;
    input2.mouse_x = 100.0;

    controller1.update(input1, camera1, 0.016f);
    controller2.update(input2, camera2, 0.016f);

    // Camera2 should have rotated more (higher sensitivity)
    float yaw1 = camera1.yaw();
    float yaw2 = camera2.yaw();

    EXPECT_GT(std::abs(yaw2 - (-90.0f)), std::abs(yaw1 - (-90.0f)));
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(CameraController_Integration, ComplexMovement_CombinesActionsCorrectly)
{
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // Press multiple keys (W + D + Q)
    InputState input{};
    input.keys.set(KeyCode::W, true); // Forward
    input.keys.set(KeyCode::D, true); // Right
    input.keys.set(KeyCode::Q, true); // Up

    controller.update(input, camera, 0.1f);

    glm::vec3 new_pos = camera.position();
    // Should have moved forward (0, 0, -1) + right (1, 0, 0) + up (0, 1, 0)
    EXPECT_TRUE(vec3_near(new_pos, glm::vec3(1.0f, 1.0f, -1.0f), 0.01f));
}

TEST(CameraController_Integration, MovementAndRotation_BothWork)
{
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // First frame: establish mouse baseline
    InputState input{};
    input.mouse_x = 0.0;
    input.mouse_y = 0.0;
    controller.update(input, camera, 0.016f);

    // Second frame: move and rotate
    input.keys.set(KeyCode::W, true);
    input.mouse_x = 100.0;
    controller.update(input, camera, 0.016f);

    // Camera should have moved forward
    EXPECT_LT(camera.position().z, 0.0f);

    // Camera should have rotated right
    EXPECT_GT(camera.yaw(), -90.0f);
}

TEST(CameraController_Integration, ZeroSpeed_NoMovement)
{
    CameraController controller = CameraController::fps_controller(0.0f, 0.1f);
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    InputState input{};
    input.keys.set(KeyCode::W, true);
    controller.update(input, camera, 0.1f);

    // Position should not have changed (speed = 0)
    EXPECT_TRUE(vec3_near(camera.position(), glm::vec3(0, 0, 0)));
}
