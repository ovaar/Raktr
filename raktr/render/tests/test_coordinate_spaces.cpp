/*!
 * @file test_coordinate_spaces.cpp
 * @brief Unit tests for strongly-typed coordinate space separation.
 */

#include "math/coordinate_spaces.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

using namespace raktr::render::math::spaces;

constexpr float EPSILON = 1e-5f;

// ============================================================================
// Vector3 Construction Tests
// ============================================================================

TEST(Vector3_Construction, FromComponents_StoresCorrectValues)
{
    LocalVector v(1.0f, 2.0f, 3.0f);

    EXPECT_FLOAT_EQ(v.x(), 1.0f);
    EXPECT_FLOAT_EQ(v.y(), 2.0f);
    EXPECT_FLOAT_EQ(v.z(), 3.0f);
}

TEST(Vector3_Construction, FromGlmVector_StoresCorrectValues)
{
    glm::vec3   glm_v(4.0f, 5.0f, 6.0f);
    WorldVector v(glm_v);

    EXPECT_FLOAT_EQ(v.x(), 4.0f);
    EXPECT_FLOAT_EQ(v.y(), 5.0f);
    EXPECT_FLOAT_EQ(v.z(), 6.0f);
}

TEST(Vector3_Construction, GlmAccessor_ReturnsCorrectVector)
{
    ViewVector       v(7.0f, 8.0f, 9.0f);
    const glm::vec3& glm_v = v.glm();

    EXPECT_FLOAT_EQ(glm_v.x, 7.0f);
    EXPECT_FLOAT_EQ(glm_v.y, 8.0f);
    EXPECT_FLOAT_EQ(glm_v.z, 9.0f);
}

// ============================================================================
// Vector3 Arithmetic Tests
// ============================================================================

TEST(Vector3_Arithmetic, Addition_SameSpace_Works)
{
    LocalVector a(1.0f, 2.0f, 3.0f);
    LocalVector b(4.0f, 5.0f, 6.0f);

    LocalVector result = a + b;

    EXPECT_FLOAT_EQ(result.x(), 5.0f);
    EXPECT_FLOAT_EQ(result.y(), 7.0f);
    EXPECT_FLOAT_EQ(result.z(), 9.0f);
}

TEST(Vector3_Arithmetic, Subtraction_SameSpace_Works)
{
    WorldVector a(10.0f, 20.0f, 30.0f);
    WorldVector b(1.0f, 2.0f, 3.0f);

    WorldVector result = a - b;

    EXPECT_FLOAT_EQ(result.x(), 9.0f);
    EXPECT_FLOAT_EQ(result.y(), 18.0f);
    EXPECT_FLOAT_EQ(result.z(), 27.0f);
}

TEST(Vector3_Arithmetic, ScalarMultiplication_Works)
{
    ViewVector v(1.0f, 2.0f, 3.0f);

    ViewVector result = v * 2.0f;

    EXPECT_FLOAT_EQ(result.x(), 2.0f);
    EXPECT_FLOAT_EQ(result.y(), 4.0f);
    EXPECT_FLOAT_EQ(result.z(), 6.0f);
}

TEST(Vector3_Arithmetic, ScalarMultiplication_Commutative)
{
    LocalVector v(2.0f, 3.0f, 4.0f);

    LocalVector result1 = v * 3.0f;
    LocalVector result2 = 3.0f * v;

    EXPECT_FLOAT_EQ(result1.x(), result2.x());
    EXPECT_FLOAT_EQ(result1.y(), result2.y());
    EXPECT_FLOAT_EQ(result1.z(), result2.z());
}

TEST(Vector3_Arithmetic, ScalarDivision_Works)
{
    WorldVector v(10.0f, 20.0f, 30.0f);

    WorldVector result = v / 10.0f;

    EXPECT_FLOAT_EQ(result.x(), 1.0f);
    EXPECT_FLOAT_EQ(result.y(), 2.0f);
    EXPECT_FLOAT_EQ(result.z(), 3.0f);
}

TEST(Vector3_Arithmetic, Negation_Works)
{
    ViewVector v(1.0f, -2.0f, 3.0f);

    ViewVector result = -v;

    EXPECT_FLOAT_EQ(result.x(), -1.0f);
    EXPECT_FLOAT_EQ(result.y(), 2.0f);
    EXPECT_FLOAT_EQ(result.z(), -3.0f);
}

TEST(Vector3_Arithmetic, InPlaceAddition_Works)
{
    LocalVector v(1.0f, 2.0f, 3.0f);
    LocalVector other(4.0f, 5.0f, 6.0f);

    v += other;

    EXPECT_FLOAT_EQ(v.x(), 5.0f);
    EXPECT_FLOAT_EQ(v.y(), 7.0f);
    EXPECT_FLOAT_EQ(v.z(), 9.0f);
}

TEST(Vector3_Arithmetic, InPlaceSubtraction_Works)
{
    WorldVector v(10.0f, 20.0f, 30.0f);
    WorldVector other(1.0f, 2.0f, 3.0f);

    v -= other;

    EXPECT_FLOAT_EQ(v.x(), 9.0f);
    EXPECT_FLOAT_EQ(v.y(), 18.0f);
    EXPECT_FLOAT_EQ(v.z(), 27.0f);
}

TEST(Vector3_Arithmetic, InPlaceScalarMultiplication_Works)
{
    ViewVector v(1.0f, 2.0f, 3.0f);

    v *= 2.0f;

    EXPECT_FLOAT_EQ(v.x(), 2.0f);
    EXPECT_FLOAT_EQ(v.y(), 4.0f);
    EXPECT_FLOAT_EQ(v.z(), 6.0f);
}

TEST(Vector3_Arithmetic, InPlaceScalarDivision_Works)
{
    LocalVector v(10.0f, 20.0f, 30.0f);

    v /= 10.0f;

    EXPECT_FLOAT_EQ(v.x(), 1.0f);
    EXPECT_FLOAT_EQ(v.y(), 2.0f);
    EXPECT_FLOAT_EQ(v.z(), 3.0f);
}

// ============================================================================
// Vector3 Geometric Operations Tests
// ============================================================================

TEST(Vector3_Geometric, DotProduct_SameSpace_Works)
{
    LocalVector a(1.0f, 0.0f, 0.0f);
    LocalVector b(0.0f, 1.0f, 0.0f);

    float dot = a.dot(b);

    EXPECT_NEAR(dot, 0.0f, EPSILON);
}

TEST(Vector3_Geometric, DotProduct_ParallelVectors_GivesLength)
{
    WorldVector a(3.0f, 4.0f, 0.0f);
    WorldVector b(3.0f, 4.0f, 0.0f);

    float dot      = a.dot(b);
    float expected = 9.0f + 16.0f; // 3*3 + 4*4

    EXPECT_NEAR(dot, expected, EPSILON);
}

TEST(Vector3_Geometric, CrossProduct_OrthogonalVectors_Works)
{
    ViewVector x(1.0f, 0.0f, 0.0f);
    ViewVector y(0.0f, 1.0f, 0.0f);

    ViewVector z = x.cross(y);

    EXPECT_NEAR(z.x(), 0.0f, EPSILON);
    EXPECT_NEAR(z.y(), 0.0f, EPSILON);
    EXPECT_NEAR(z.z(), 1.0f, EPSILON);
}

TEST(Vector3_Geometric, Length_GivesCorrectMagnitude)
{
    LocalVector v(3.0f, 4.0f, 0.0f);

    float length = v.length();

    EXPECT_NEAR(length, 5.0f, EPSILON); // sqrt(9 + 16) = 5
}

TEST(Vector3_Geometric, LengthSquared_AvoidsSqrt)
{
    WorldVector v(3.0f, 4.0f, 0.0f);

    float length_sq = v.length_squared();

    EXPECT_NEAR(length_sq, 25.0f, EPSILON); // 9 + 16 = 25
}

TEST(Vector3_Geometric, Normalized_ProducesUnitVector)
{
    ViewVector v(3.0f, 4.0f, 0.0f);

    ViewVector unit = v.normalized();

    EXPECT_NEAR(unit.length(), 1.0f, EPSILON);
    EXPECT_NEAR(unit.x(), 0.6f, EPSILON);
    EXPECT_NEAR(unit.y(), 0.8f, EPSILON);
}

TEST(Vector3_Geometric, Normalize_ModifiesInPlace)
{
    LocalVector v(3.0f, 4.0f, 0.0f);

    v.normalize();

    EXPECT_NEAR(v.length(), 1.0f, EPSILON);
    EXPECT_NEAR(v.x(), 0.6f, EPSILON);
    EXPECT_NEAR(v.y(), 0.8f, EPSILON);
}

// ============================================================================
// SpaceTransform Tests
// ============================================================================

TEST(SpaceTransform_Basic, IdentityTransform_PreservesVector)
{
    glm::mat4    identity(1.0f);
    LocalToWorld transform(identity);

    LocalVector local(1.0f, 2.0f, 3.0f);
    WorldVector world = transform * local;

    EXPECT_NEAR(world.x(), local.x(), EPSILON);
    EXPECT_NEAR(world.y(), local.y(), EPSILON);
    EXPECT_NEAR(world.z(), local.z(), EPSILON);
}

TEST(SpaceTransform_Basic, TranslationTransform_OffsetsVector)
{
    glm::mat4    translation = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 10.0f, -3.0f));
    LocalToWorld transform(translation);

    LocalVector local(1.0f, 2.0f, 3.0f);
    WorldVector world = transform * local;

    EXPECT_NEAR(world.x(), 6.0f, EPSILON);  // 1 + 5
    EXPECT_NEAR(world.y(), 12.0f, EPSILON); // 2 + 10
    EXPECT_NEAR(world.z(), 0.0f, EPSILON);  // 3 - 3
}

TEST(SpaceTransform_Basic, RotationTransform_RotatesVector)
{
    // Rotate 90° around Y axis
    glm::mat4    rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
    LocalToWorld transform(rotation);

    LocalVector local(1.0f, 0.0f, 0.0f); // X axis
    WorldVector world = transform * local;

    // After 90° Y rotation: X -> -Z
    EXPECT_NEAR(world.x(), 0.0f, EPSILON);
    EXPECT_NEAR(world.y(), 0.0f, EPSILON);
    EXPECT_NEAR(world.z(), -1.0f, EPSILON);
}

TEST(SpaceTransform_Basic, ScaleTransform_ScalesVector)
{
    glm::mat4    scale = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f));
    LocalToWorld transform(scale);

    LocalVector local(1.0f, 1.0f, 1.0f);
    WorldVector world = transform * local;

    EXPECT_NEAR(world.x(), 2.0f, EPSILON);
    EXPECT_NEAR(world.y(), 3.0f, EPSILON);
    EXPECT_NEAR(world.z(), 4.0f, EPSILON);
}

TEST(SpaceTransform_Direction, TransformDirection_IgnoresTranslation)
{
    glm::mat4    translation = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 10.0f, -3.0f));
    LocalToWorld transform(translation);

    LocalVector direction(1.0f, 0.0f, 0.0f);
    WorldVector world_direction = transform.transform_direction(direction);

    // Direction should not be affected by translation
    EXPECT_NEAR(world_direction.x(), 1.0f, EPSILON);
    EXPECT_NEAR(world_direction.y(), 0.0f, EPSILON);
    EXPECT_NEAR(world_direction.z(), 0.0f, EPSILON);
}

// ============================================================================
// SpaceTransform Composition Tests
// ============================================================================

TEST(SpaceTransform_Composition, TwoTransforms_ComposeCorrectly)
{
    // Local -> World: translate by (1, 0, 0)
    glm::mat4    model = glm::translate(glm::mat4(1.0f), glm::vec3(1, 0, 0));
    LocalToWorld local_to_world(model);

    // World -> View: translate by (0, 2, 0)
    glm::mat4   view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 2, 0));
    WorldToView world_to_view(view);

    // Compose: Local -> World -> View
    LocalToView local_to_view = world_to_view * local_to_world;

    LocalVector local(0, 0, 0);
    ViewVector  view_pos = local_to_view * local;

    EXPECT_NEAR(view_pos.x(), 1.0f, EPSILON);
    EXPECT_NEAR(view_pos.y(), 2.0f, EPSILON);
    EXPECT_NEAR(view_pos.z(), 0.0f, EPSILON);
}

TEST(SpaceTransform_Composition, ThreeTransforms_MatchManualComposition)
{
    // Model transform
    glm::mat4    model = glm::translate(glm::mat4(1.0f), glm::vec3(1, 0, 0));
    LocalToWorld local_to_world(model);

    // View transform
    glm::mat4   view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 1, 0));
    WorldToView world_to_view(view);

    // Projection transform
    glm::mat4  projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    ViewToClip view_to_clip(projection);

    // Compose all
    LocalToClip local_to_clip = view_to_clip * world_to_view * local_to_world;

    // Manual composition
    glm::mat4   mvp = projection * view * model;
    LocalToClip manual_local_to_clip(mvp);

    // Transform same vector both ways
    LocalVector local(0.5f, 0.5f, -5.0f);
    ClipVector  result1 = local_to_clip * local;
    ClipVector  result2 = manual_local_to_clip * local;

    EXPECT_NEAR(result1.x(), result2.x(), EPSILON);
    EXPECT_NEAR(result1.y(), result2.y(), EPSILON);
    EXPECT_NEAR(result1.z(), result2.z(), EPSILON);
}

// ============================================================================
// Type Safety Compilation Tests (These verify compile-time safety)
// ============================================================================

TEST(TypeSafety_Compilation, SameSpace_OperationsCompile)
{
    // These should all compile fine
    LocalVector a(1, 2, 3);
    LocalVector b(4, 5, 6);

    [[maybe_unused]] LocalVector sum   = a + b;
    [[maybe_unused]] LocalVector diff  = a - b;
    [[maybe_unused]] float       dot   = a.dot(b);
    [[maybe_unused]] LocalVector cross = a.cross(b);

    SUCCEED(); // If we got here, it compiled
}

TEST(TypeSafety_Compilation, CorrectSpaceTransform_Compiles)
{
    LocalVector  local(1, 2, 3);
    LocalToWorld transform(glm::mat4(1.0f));

    [[maybe_unused]] WorldVector world = transform * local;

    SUCCEED(); // If we got here, it compiled
}

// ============================================================================
// Real-World Usage Tests
// ============================================================================

TEST(RealWorld_Usage, ModelViewProjection_Pipeline)
{
    // Setup transforms
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -5));
    glm::mat4 view  = glm::lookAt(
        glm::vec3(0, 0, 0),  // eye
        glm::vec3(0, 0, -1), // target
        glm::vec3(0, 1, 0)   // up
    );
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    LocalToWorld model_transform(model);
    WorldToView  view_transform(view);
    ViewToClip   projection_transform(projection);

    // Local space vertex
    LocalVector vertex(0, 0, 0);

    // Transform through pipeline
    WorldVector world_vertex = model_transform * vertex;
    ViewVector  view_vertex  = view_transform * world_vertex;
    ClipVector  clip_vertex  = projection_transform * view_vertex;

    // Verify it went through the pipeline
    EXPECT_TRUE(std::isfinite(clip_vertex.x()));
    EXPECT_TRUE(std::isfinite(clip_vertex.y()));
    EXPECT_TRUE(std::isfinite(clip_vertex.z()));
}

TEST(RealWorld_Usage, DirectionVector_NotAffectedByTranslation)
{
    // Model transform with translation
    glm::mat4    model = glm::translate(glm::mat4(1.0f), glm::vec3(10, 20, 30));
    LocalToWorld transform(model);

    // Normal vector (direction)
    LocalVector normal(0, 1, 0);

    // Transform as direction
    WorldVector world_normal = transform.transform_direction(normal);

    // Should still be unit Y, not affected by translation
    EXPECT_NEAR(world_normal.x(), 0.0f, EPSILON);
    EXPECT_NEAR(world_normal.y(), 1.0f, EPSILON);
    EXPECT_NEAR(world_normal.z(), 0.0f, EPSILON);
}

TEST(RealWorld_Usage, ComposedTransform_MatchesDirectApplication)
{
    // Build transforms
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0));
    glm::mat4 view  = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -10));

    LocalToWorld l2w(model);
    WorldToView  w2v(view);

    // Composed transform
    LocalToView l2v = w2v * l2w;

    // Direct transform
    glm::mat4   combined = view * model;
    LocalToView direct_l2v(combined);

    // Transform same vector
    LocalVector local(1, 0, 0);
    ViewVector  composed_result = l2v * local;
    ViewVector  direct_result   = direct_l2v * local;

    EXPECT_NEAR(composed_result.x(), direct_result.x(), EPSILON);
    EXPECT_NEAR(composed_result.y(), direct_result.y(), EPSILON);
    EXPECT_NEAR(composed_result.z(), direct_result.z(), EPSILON);
}
