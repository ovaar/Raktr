/*!
 * @file test_coordinate_system.cpp
 * @brief Unit tests for right-hand coordinate system utilities.
 */

#include "math/coordinate_system.h"
#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace raktr::render::math;

constexpr float EPSILON = 1e-6f;

// ============================================================================
// Standard Basis Tests
// ============================================================================

TEST(CoordinateBasis_Standard, Right_IsPositiveX)
{
    auto right = CoordinateBasis::Standard::right();
    EXPECT_FLOAT_EQ(right.x, 1.0f);
    EXPECT_FLOAT_EQ(right.y, 0.0f);
    EXPECT_FLOAT_EQ(right.z, 0.0f);
}

TEST(CoordinateBasis_Standard, Up_IsPositiveY)
{
    auto up = CoordinateBasis::Standard::up();
    EXPECT_FLOAT_EQ(up.x, 0.0f);
    EXPECT_FLOAT_EQ(up.y, 1.0f);
    EXPECT_FLOAT_EQ(up.z, 0.0f);
}

TEST(CoordinateBasis_Standard, Forward_IsNegativeZ)
{
    auto forward = CoordinateBasis::Standard::forward();
    EXPECT_FLOAT_EQ(forward.x, 0.0f);
    EXPECT_FLOAT_EQ(forward.y, 0.0f);
    EXPECT_FLOAT_EQ(forward.z, -1.0f);
}

TEST(CoordinateBasis_Standard, FormsRightHandedBasis)
{
    auto right = CoordinateBasis::Standard::right();
    auto up = CoordinateBasis::Standard::up();
    auto forward = CoordinateBasis::Standard::forward();
    
    EXPECT_TRUE(CoordinateBasis::is_right_handed(right, up, forward));
}

// ============================================================================
// Right-Hand Rule Validation Tests
// ============================================================================

TEST(CoordinateBasis_IsRightHanded, StandardXYZ_IsRightHanded)
{
    // Standard OpenGL/Vulkan: X right, Y up, -Z forward
    glm::vec3 right(1, 0, 0);
    glm::vec3 up(0, 1, 0);
    glm::vec3 forward(0, 0, -1);
    
    EXPECT_TRUE(CoordinateBasis::is_right_handed(right, up, forward));
}

TEST(CoordinateBasis_IsRightHanded, FlippedForward_IsNotRightHanded)
{
    glm::vec3 right(1, 0, 0);
    glm::vec3 up(0, 1, 0);
    glm::vec3 forward(0, 0, 1); // Wrong! Should be -Z for right-hand
    
    EXPECT_FALSE(CoordinateBasis::is_right_handed(right, up, forward));
}

TEST(CoordinateBasis_IsRightHanded, RotatedBasis_StillRightHanded)
{
    // Rotate 45° around Y axis using GLM
    float angle = glm::radians(45.0f);
    glm::mat4 rotation = glm::rotate(glm::mat4(1), angle, glm::vec3(0, 1, 0));
    
    // Transform standard basis
    glm::vec3 right = glm::vec3(rotation * glm::vec4(1, 0, 0, 0));
    glm::vec3 up = glm::vec3(rotation * glm::vec4(0, 1, 0, 0));
    glm::vec3 forward = glm::vec3(rotation * glm::vec4(0, 0, -1, 0));
    
    EXPECT_TRUE(CoordinateBasis::is_right_handed(right, up, forward));
}

TEST(CoordinateBasis_IsRightHanded, PermutedAxes_ValidatesCorrectly)
{
    // Z, X, Y permutation: right=Z, up=X, forward=Y
    glm::vec3 right(0, 0, 1);
    glm::vec3 up(1, 0, 0);
    
    // Our is_right_handed expects: forward = -(right × up)
    auto computed_forward = -glm::cross(right, up);
    EXPECT_NEAR(computed_forward.x, 0.0f, EPSILON);
    EXPECT_NEAR(computed_forward.y, -1.0f, EPSILON);  // -(0,1,0) = (0,-1,0)
    EXPECT_NEAR(computed_forward.z, 0.0f, EPSILON);
    
    // Use the negated forward
    EXPECT_TRUE(CoordinateBasis::is_right_handed(right, up, computed_forward));
}

// ============================================================================
// Forward Computation Tests
// ============================================================================

TEST(CoordinateBasis_ForwardFromRightUp, StandardAxes_GiveNegativeZ)
{
    glm::vec3 right(1, 0, 0);
    glm::vec3 up(0, 1, 0);
    
    auto forward = CoordinateBasis::forward_from_right_up(right, up);
    
    EXPECT_NEAR(forward.x, 0.0f, EPSILON);
    EXPECT_NEAR(forward.y, 0.0f, EPSILON);
    EXPECT_NEAR(forward.z, -1.0f, EPSILON);
}

TEST(CoordinateBasis_ForwardFromRightUp, NormalizesResult)
{
    glm::vec3 right(2, 0, 0);  // Not normalized
    glm::vec3 up(0, 3, 0);     // Not normalized
    
    auto forward = CoordinateBasis::forward_from_right_up(right, up);
    
    float length = glm::length(forward);
    EXPECT_NEAR(length, 1.0f, EPSILON);
}

TEST(CoordinateBasis_ForwardFromRightUp, RotatedBasis_GivesRotatedForward)
{
    // Rotate +30° around Y
    float angle = glm::radians(30.0f);
    glm::mat4 rotation = glm::rotate(glm::mat4(1), angle, glm::vec3(0, 1, 0));
    
    glm::vec3 right = glm::vec3(rotation * glm::vec4(1, 0, 0, 0));
    glm::vec3 up = glm::vec3(rotation * glm::vec4(0, 1, 0, 0));
    glm::vec3 expected_forward = glm::vec3(rotation * glm::vec4(0, 0, -1, 0));
    
    auto forward = CoordinateBasis::forward_from_right_up(right, up);
    
    EXPECT_NEAR(forward.x, expected_forward.x, 1e-5f);
    EXPECT_NEAR(forward.y, expected_forward.y, 1e-5f);
    EXPECT_NEAR(forward.z, expected_forward.z, 1e-5f);
}

// ============================================================================
// Up Computation Tests
// ============================================================================

TEST(CoordinateBasis_UpFromForwardRight, StandardAxes_GivePositiveY)
{
    glm::vec3 forward(0, 0, -1);
    glm::vec3 right(1, 0, 0);
    
    auto up = CoordinateBasis::up_from_forward_right(forward, right);
    
    EXPECT_NEAR(up.x, 0.0f, EPSILON);
    EXPECT_NEAR(up.y, 1.0f, EPSILON);
    EXPECT_NEAR(up.z, 0.0f, EPSILON);
}

TEST(CoordinateBasis_UpFromForwardRight, NormalizesResult)
{
    glm::vec3 forward(0, 0, -2);
    glm::vec3 right(3, 0, 0);
    
    auto up = CoordinateBasis::up_from_forward_right(forward, right);
    
    float length = glm::length(up);
    EXPECT_NEAR(length, 1.0f, EPSILON);
}

// ============================================================================
// Right Computation Tests
// ============================================================================

TEST(CoordinateBasis_RightFromUpForward, StandardAxes_GivePositiveX)
{
    glm::vec3 up(0, 1, 0);
    glm::vec3 forward(0, 0, -1);
    
    auto right = CoordinateBasis::right_from_up_forward(up, forward);
    
    EXPECT_NEAR(right.x, 1.0f, EPSILON);
    EXPECT_NEAR(right.y, 0.0f, EPSILON);
    EXPECT_NEAR(right.z, 0.0f, EPSILON);
}

TEST(CoordinateBasis_RightFromUpForward, NormalizesResult)
{
    glm::vec3 up(0, 2, 0);
    glm::vec3 forward(0, 0, -3);
    
    auto right = CoordinateBasis::right_from_up_forward(up, forward);
    
    float length = glm::length(right);
    EXPECT_NEAR(length, 1.0f, EPSILON);
}

// ============================================================================
// Orthogonality Tests
// ============================================================================

TEST(CoordinateBasis_Orthogonality, ComputedAxes_ArePerpendicular)
{
    glm::vec3 right(1, 0, 0);
    glm::vec3 up(0, 1, 0);
    auto forward = CoordinateBasis::forward_from_right_up(right, up);
    
    // All axes should be perpendicular (dot product = 0)
    EXPECT_NEAR(glm::dot(right, up), 0.0f, EPSILON);
    EXPECT_NEAR(glm::dot(right, forward), 0.0f, EPSILON);
    EXPECT_NEAR(glm::dot(up, forward), 0.0f, EPSILON);
}

TEST(CoordinateBasis_Orthogonality, ComputedFromArbitraryVectors)
{
    // Start with arbitrary (but roughly perpendicular) vectors
    glm::vec3 approx_right(1, 0.1f, 0);
    glm::vec3 approx_up(0, 1, 0.1f);
    
    // Compute orthonormal basis
    auto right = glm::normalize(approx_right);
    auto up = glm::normalize(approx_up);
    auto forward = CoordinateBasis::forward_from_right_up(right, up);
    
    // Recompute up to ensure perfect orthogonality
    up = CoordinateBasis::up_from_forward_right(forward, right);
    
    // Verify all perpendicular
    EXPECT_NEAR(glm::dot(right, up), 0.0f, 1e-5f);
    EXPECT_NEAR(glm::dot(right, forward), 0.0f, 1e-5f);
    EXPECT_NEAR(glm::dot(up, forward), 0.0f, 1e-5f);
    
    // Verify right-handed
    EXPECT_TRUE(CoordinateBasis::is_right_handed(right, up, forward));
}

// ============================================================================
// Cross Product Identity Tests
// ============================================================================

TEST(CoordinateBasis_CrossProduct, RightCrossUp_EqualsNegativeForward)
{
    auto right = CoordinateBasis::Standard::right();
    auto up = CoordinateBasis::Standard::up();
    auto expected_forward = CoordinateBasis::Standard::forward();
    
    // In OpenGL/Vulkan: right × up = +Z, but forward is -Z
    auto computed = glm::cross(right, up);
    
    EXPECT_NEAR(computed.x, -expected_forward.x, EPSILON);
    EXPECT_NEAR(computed.y, -expected_forward.y, EPSILON);
    EXPECT_NEAR(computed.z, -expected_forward.z, EPSILON);
}

TEST(CoordinateBasis_CrossProduct, UpCrossForward_EqualsNegativeRight)
{
    auto up = CoordinateBasis::Standard::up();
    auto forward = CoordinateBasis::Standard::forward();
    auto expected_right = CoordinateBasis::Standard::right();
    
    // In OpenGL/Vulkan: up × forward = -X (because forward is -Z)
    auto computed = glm::cross(up, forward);
    
    EXPECT_NEAR(computed.x, -expected_right.x, EPSILON);
    EXPECT_NEAR(computed.y, -expected_right.y, EPSILON);
    EXPECT_NEAR(computed.z, -expected_right.z, EPSILON);
}

TEST(CoordinateBasis_CrossProduct, ForwardCrossRight_EqualsNegativeUp)
{
    auto forward = CoordinateBasis::Standard::forward();
    auto right = CoordinateBasis::Standard::right();
    auto expected_up = CoordinateBasis::Standard::up();
    
    // In OpenGL/Vulkan: forward × right = -Y (because forward is -Z)
    auto computed = glm::cross(forward, right);
    
    EXPECT_NEAR(computed.x, -expected_up.x, EPSILON);
    EXPECT_NEAR(computed.y, -expected_up.y, EPSILON);
    EXPECT_NEAR(computed.z, -expected_up.z, EPSILON);
}

// ============================================================================
// Real-World Usage Tests
// ============================================================================

TEST(CoordinateBasis_RealWorld, CameraLookAt_ProducesRightHandedBasis)
{
    // Simulate camera setup
    glm::vec3 camera_pos(0, 2, 5);
    glm::vec3 look_at_target(0, 0, 0);
    glm::vec3 world_up(0, 1, 0);
    
    // Compute camera basis (this is view space)
    glm::vec3 view_forward = glm::normalize(camera_pos - look_at_target);  // Camera looks at target
    glm::vec3 right = glm::normalize(glm::cross(world_up, view_forward));
    glm::vec3 up = glm::cross(view_forward, right);
    
    // In view space, forward is +Z (looking down +Z), but we want -Z convention
    glm::vec3 forward = -view_forward;
    
    // Verify right-handed with OpenGL convention
    EXPECT_TRUE(CoordinateBasis::is_right_handed(right, up, forward));
}

TEST(CoordinateBasis_RealWorld, ModelTransform_PreservesHandedness)
{
    // Start with standard basis
    auto right = CoordinateBasis::Standard::right();
    auto up = CoordinateBasis::Standard::up();
    auto forward = CoordinateBasis::Standard::forward();
    
    // Rotate 90° around Y (right-hand rule: counter-clockwise when looking down Y)
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(0, 1, 0));
    
    // Transform basis vectors
    glm::vec3 new_right = glm::vec3(rotation * glm::vec4(right, 0));
    glm::vec3 new_up = glm::vec3(rotation * glm::vec4(up, 0));
    glm::vec3 new_forward = glm::vec3(rotation * glm::vec4(forward, 0));
    
    // Should still be right-handed
    EXPECT_TRUE(CoordinateBasis::is_right_handed(new_right, new_up, new_forward));
}

TEST(CoordinateBasis_RealWorld, ComputeForwardFromCamera_MatchesStandard)
{
    // Camera looking down -Z (standard OpenGL)
    glm::vec3 right(1, 0, 0);
    glm::vec3 up(0, 1, 0);
    
    auto forward = CoordinateBasis::forward_from_right_up(right, up);
    
    // Should match standard forward
    auto std_forward = CoordinateBasis::Standard::forward();
    EXPECT_NEAR(forward.x, std_forward.x, EPSILON);
    EXPECT_NEAR(forward.y, std_forward.y, EPSILON);
    EXPECT_NEAR(forward.z, std_forward.z, EPSILON);
}
