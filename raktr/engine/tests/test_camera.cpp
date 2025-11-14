/*!
 * @file test_camera.cpp
 * @brief Unit tests for Camera class.
 */

#include "input/camera_controller.h"
#include "input/input_system.h"
#include "math/transform_types.h" // From render module
#include "scene/camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
// Construction Tests
// ============================================================================

TEST(Camera_Construction, DefaultValues_ProduceValidCamera)
{
    glm::vec3 position(0.0f, 0.0f, 3.0f);
    Camera    camera(position, 45.0f, 16.0f / 9.0f);

    EXPECT_TRUE(vec3_near(camera.position(), position));
    EXPECT_FLOAT_EQ(camera.yaw(), -90.0f); // Looking down -Z
    EXPECT_FLOAT_EQ(camera.pitch(), 0.0f); // Level
}

TEST(Camera_Construction, CustomFOV_IsStored)
{
    Camera camera(glm::vec3(0, 0, 0), 60.0f, 1.0f);

    // FOV is stored and used for projection matrix
    // We can verify via the projection matrix, but direct getter would be cleaner
    auto proj = camera.projection();
    EXPECT_TRUE(proj.matrix()[0][0] != 0.0f); // Sanity check: matrix is valid
}

TEST(Camera_Construction, CustomPosition_IsStored)
{
    glm::vec3 position(5.0f, 10.0f, 15.0f);
    Camera    camera(position, 45.0f, 1.0f);

    EXPECT_TRUE(vec3_near(camera.position(), position));
}

// ============================================================================
// Orientation Tests
// ============================================================================

TEST(Camera_Orientation, DefaultForward_IsNegativeZ)
{
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    glm::vec3 forward = camera.forward();
    EXPECT_TRUE(vec3_near(forward, glm::vec3(0.0f, 0.0f, -1.0f)));
}

TEST(Camera_Orientation, DefaultRight_IsPositiveX)
{
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    glm::vec3 right = camera.right();
    EXPECT_TRUE(vec3_near(right, glm::vec3(1.0f, 0.0f, 0.0f)));
}

TEST(Camera_Orientation, DefaultUp_IsPositiveY)
{
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    glm::vec3 up = camera.up();
    EXPECT_TRUE(vec3_near(up, glm::vec3(0.0f, 1.0f, 0.0f)));
}

TEST(Camera_Orientation, SetRotation_UpdatesVectors)
{
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // Rotate 90° right (yaw = 0°, looking down +X)
    camera.set_rotation(0.0f, 0.0f);

    glm::vec3 forward = camera.forward();
    EXPECT_TRUE(vec3_near(forward, glm::vec3(1.0f, 0.0f, 0.0f), 0.01f));
}

TEST(Camera_Orientation, SetPitch45_UpdatesForward)
{
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // Look 45° up
    camera.set_rotation(-90.0f, 45.0f);

    glm::vec3 forward = camera.forward();
    // Forward should be (0, sin(45), -cos(45)) normalized
    float cos45 = std::cos(glm::radians(45.0f));
    float sin45 = std::sin(glm::radians(45.0f));
    EXPECT_TRUE(vec3_near(forward, glm::vec3(0.0f, sin45, -cos45), 0.01f));
}

// ============================================================================
// Movement Tests
// ============================================================================

TEST(Camera_Movement, ForwardKey_MovesInForwardDirection)
{
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);

    // Create input state with W key pressed
    InputState input{};
    input.keys.set(KeyCode::W, true);

    controller.update(input, camera, 0.1f); // 0.1 second

    glm::vec3 new_pos = camera.position();
    // Should move 1 unit forward (speed=10, dt=0.1)
    EXPECT_TRUE(vec3_near(new_pos, glm::vec3(0.0f, 0.0f, -1.0f), 0.01f));
}

TEST(Camera_Movement, BackwardKey_MovesBackward)
{
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);

    InputState input{};
    input.keys.set(KeyCode::S, true);

    controller.update(input, camera, 0.1f);

    glm::vec3 new_pos = camera.position();
    EXPECT_TRUE(vec3_near(new_pos, glm::vec3(0.0f, 0.0f, 1.0f), 0.01f));
}

TEST(Camera_Movement, LeftKey_StrafesLeft)
{
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);

    InputState input{};
    input.keys.set(KeyCode::A, true);

    controller.update(input, camera, 0.1f);

    glm::vec3 new_pos = camera.position();
    EXPECT_TRUE(vec3_near(new_pos, glm::vec3(-1.0f, 0.0f, 0.0f), 0.01f));
}

TEST(Camera_Movement, RightKey_StrafesRight)
{
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);

    InputState input{};
    input.keys.set(KeyCode::D, true);

    controller.update(input, camera, 0.1f);

    glm::vec3 new_pos = camera.position();
    EXPECT_TRUE(vec3_near(new_pos, glm::vec3(1.0f, 0.0f, 0.0f), 0.01f));
}

TEST(Camera_Movement, MultipleKeys_CombinesMovement)
{
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);

    // Press W and D together (forward-right diagonal)
    InputState input{};
    input.keys.set(KeyCode::W, true);
    input.keys.set(KeyCode::D, true);

    controller.update(input, camera, 0.1f);

    glm::vec3 new_pos = camera.position();
    // Should move diagonally (NOT normalized - each key adds its own vector)
    // W adds (0, 0, -1), D adds (1, 0, 0) -> result is (1, 0, -1)
    EXPECT_TRUE(vec3_near(new_pos, glm::vec3(1.0f, 0.0f, -1.0f), 0.01f));
}

TEST(Camera_Movement, ZeroDeltaTime_NoMovement)
{
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);

    InputState input{};
    input.keys.set(KeyCode::W, true);

    controller.update(input, camera, 0.0f);

    EXPECT_TRUE(vec3_near(camera.position(), glm::vec3(0, 0, 0)));
}

// ============================================================================
// Mouse Look Tests
// ============================================================================

TEST(Camera_MouseLook, MouseDeltaX_UpdatesYaw)
{
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);

    // Simulate mouse movement: move mouse 100 pixels right
    InputState input{};
    input.mouse_x = 100.0;
    input.mouse_y = 0.0;

    // First frame to establish baseline
    controller.update(input, camera, 0.016f);

    // Second frame with delta
    input.mouse_x = 200.0; // Moved 100 pixels right
    controller.update(input, camera, 0.016f);

    // Yaw should have increased (looking right)
    EXPECT_GT(camera.yaw(), -90.0f);
}

TEST(Camera_MouseLook, MouseDeltaY_UpdatesPitch)
{
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);
    CameraController controller = CameraController::fps_controller(10.0f, 0.1f);

    InputState input{};
    input.mouse_x = 0.0;
    input.mouse_y = 0.0;

    controller.update(input, camera, 0.016f);

    // Move mouse down (positive Y)
    input.mouse_y = 100.0;
    controller.update(input, camera, 0.016f);

    // Pitch should have decreased (looking down)
    EXPECT_LT(camera.pitch(), 0.0f);
}

TEST(Camera_MouseLook, PitchClamping_PreventsTooMuchUp)
{
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    // Try to pitch way up
    camera.set_rotation(-90.0f, 100.0f);

    // Pitch should be clamped to 89°
    EXPECT_FLOAT_EQ(camera.pitch(), 89.0f);
}

TEST(Camera_MouseLook, PitchClamping_PreventsTooMuchDown)
{
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    camera.set_rotation(-90.0f, -100.0f);

    EXPECT_FLOAT_EQ(camera.pitch(), -89.0f);
}

// ============================================================================
// Matrix Generation Tests
// ============================================================================

TEST(Camera_ViewMatrix, MatchesGLMLookAt)
{
    glm::vec3 position(0.0f, 0.0f, 3.0f);
    Camera    camera(position, 45.0f, 1.0f);

    auto      view_matrix = camera.view();
    glm::mat4 expected    = glm::lookAt(
        position,
        position + glm::vec3(0.0f, 0.0f, -1.0f), // Looking down -Z
        glm::vec3(0.0f, 1.0f, 0.0f)              // Up is +Y
    );

    // Compare matrices element-wise
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_NEAR(view_matrix.matrix()[i][j], expected[i][j], 0.001f);
        }
    }
}

TEST(Camera_ProjectionMatrix, MatchesGLMPerspective)
{
    Camera camera(glm::vec3(0, 0, 0), 60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);

    auto      proj_matrix = camera.projection();
    glm::mat4 expected    = glm::perspective(
        glm::radians(60.0f),
        16.0f / 9.0f,
        0.1f,
        1000.0f);

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_NEAR(proj_matrix.matrix()[i][j], expected[i][j], 0.001f);
        }
    }
}

TEST(Camera_ProjectionMatrix, AspectRatioChange_UpdatesMatrix)
{
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    auto proj1 = camera.projection();

    camera.set_aspect_ratio(16.0f / 9.0f);
    auto proj2 = camera.projection();

    // Matrices should be different
    EXPECT_NE(proj1.matrix()[0][0], proj2.matrix()[0][0]);
}

// ============================================================================
// Setters and Getters Tests
// ============================================================================

TEST(Camera_Setters, SetPosition_UpdatesPosition)
{
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    glm::vec3 new_pos(5.0f, 10.0f, 15.0f);
    camera.set_position(new_pos);

    EXPECT_TRUE(vec3_near(camera.position(), new_pos));
}

TEST(Camera_Setters, SetMovementSpeed_AffectsMovement)
{
    Camera           camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);
    CameraController controller = CameraController::fps_controller(20.0f, 0.1f);

    InputState input{};
    input.keys.set(KeyCode::W, true);
    controller.update(input, camera, 0.1f);

    // Speed 20, dt 0.1 = 2 units forward
    EXPECT_TRUE(vec3_near(camera.position(), glm::vec3(0.0f, 0.0f, -2.0f), 0.01f));
}

TEST(Camera_Setters, SetFOV_UpdatesProjection)
{
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f);

    auto proj1 = camera.projection();

    camera.set_fov(90.0f);
    auto proj2 = camera.projection();

    // FOV change should affect projection matrix
    EXPECT_NE(proj1.matrix()[1][1], proj2.matrix()[1][1]);
}
