/*!
 * @file test_transform_types.cpp
 * @brief Unit tests for strongly-typed transformation wrappers.
 */

#include "math/transform_types.h"
#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>

using namespace raktr::render::math;

// Helper to compare matrices with tolerance
constexpr float EPSILON = 1e-5f;

bool matrices_equal(const glm::mat4& a, const glm::mat4& b, float epsilon = EPSILON)
{
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            if (std::abs(a[col][row] - b[col][row]) > epsilon)
            {
                return false;
            }
        }
    }
    return true;
}

// ============================================================================
// Axis Tests
// ============================================================================

TEST(Axis_Construction, XYZ_StaticMethods_ProduceUnitVectors)
{
    auto x = Axis::X();
    auto y = Axis::Y();
    auto z = Axis::Z();

    EXPECT_FLOAT_EQ(x.vector().x, 1.0f);
    EXPECT_FLOAT_EQ(x.vector().y, 0.0f);
    EXPECT_FLOAT_EQ(x.vector().z, 0.0f);

    EXPECT_FLOAT_EQ(y.vector().x, 0.0f);
    EXPECT_FLOAT_EQ(y.vector().y, 1.0f);
    EXPECT_FLOAT_EQ(y.vector().z, 0.0f);

    EXPECT_FLOAT_EQ(z.vector().x, 0.0f);
    EXPECT_FLOAT_EQ(z.vector().y, 0.0f);
    EXPECT_FLOAT_EQ(z.vector().z, 1.0f);
}

TEST(Axis_Construction, FromVector_AutomaticallyNormalizes)
{
    Axis axis(3.0f, 4.0f, 0.0f);
    
    // Should be normalized (length = 1)
    float length = glm::length(axis.vector());
    EXPECT_NEAR(length, 1.0f, EPSILON);

    // Check correct direction (3,4,0 normalized is 0.6, 0.8, 0)
    EXPECT_NEAR(axis.vector().x, 0.6f, EPSILON);
    EXPECT_NEAR(axis.vector().y, 0.8f, EPSILON);
    EXPECT_NEAR(axis.vector().z, 0.0f, EPSILON);
}

TEST(Axis_Construction, FromGlmVec3_AutomaticallyNormalizes)
{
    glm::vec3 v(1.0f, 1.0f, 1.0f);
    Axis axis(v);

    float length = glm::length(axis.vector());
    EXPECT_NEAR(length, 1.0f, EPSILON);

    // (1,1,1) normalized is (0.577, 0.577, 0.577)
    float expected = 1.0f / std::sqrt(3.0f);
    EXPECT_NEAR(axis.vector().x, expected, EPSILON);
    EXPECT_NEAR(axis.vector().y, expected, EPSILON);
    EXPECT_NEAR(axis.vector().z, expected, EPSILON);
}

// ============================================================================
// Rotation Tests
// ============================================================================

TEST(Rotation_Construction, Identity_ProducesIdentityMatrix)
{
    auto rot = Rotation::identity();
    
    glm::mat4 expected = glm::mat4(1.0f);
    EXPECT_TRUE(matrices_equal(rot.matrix(), expected));
}

TEST(Rotation_Construction, ZeroAngle_ProducesIdentityMatrix)
{
    Rotation rot(0.0f, Axis::Y());
    
    glm::mat4 expected = glm::mat4(1.0f);
    EXPECT_TRUE(matrices_equal(rot.matrix(), expected));
}

TEST(Rotation_Construction, QuarterTurnAroundY_MatchesGLM)
{
    const float angle = glm::half_pi<float>(); // 90 degrees
    Rotation rot(angle, Axis::Y());

    glm::mat4 expected = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_TRUE(matrices_equal(rot.matrix(), expected));
}

TEST(Rotation_Construction, FromRawVector_NormalizesAxis)
{
    // Non-normalized axis
    glm::vec3 axis(2.0f, 0.0f, 0.0f);
    Rotation rot(glm::half_pi<float>(), axis);

    // Should produce same result as normalized X axis
    glm::mat4 expected = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
    EXPECT_TRUE(matrices_equal(rot.matrix(), expected));
}

TEST(Rotation_ToBytes, ProducesCorrectByteSpan)
{
    Rotation rot(0.0f, Axis::Y());
    
    auto bytes = rot.to_bytes();
    EXPECT_EQ(bytes.size(), 16 * sizeof(float));
}

// ============================================================================
// Translation Tests
// ============================================================================

TEST(Translation_Construction, Identity_ProducesIdentityMatrix)
{
    auto trans = Translation::identity();
    
    glm::mat4 expected = glm::mat4(1.0f);
    EXPECT_TRUE(matrices_equal(trans.matrix(), expected));
}

TEST(Translation_Construction, FromCoordinates_MatchesGLM)
{
    Translation trans(1.0f, 2.0f, 3.0f);

    glm::mat4 expected = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_TRUE(matrices_equal(trans.matrix(), expected));
}

TEST(Translation_Construction, FromVector_MatchesGLM)
{
    glm::vec3 offset(5.0f, -2.0f, 10.0f);
    Translation trans(offset);

    glm::mat4 expected = glm::translate(glm::mat4(1.0f), offset);
    EXPECT_TRUE(matrices_equal(trans.matrix(), expected));
}

TEST(Translation_ToBytes, ProducesCorrectByteSpan)
{
    Translation trans(1.0f, 2.0f, 3.0f);
    
    auto bytes = trans.to_bytes();
    EXPECT_EQ(bytes.size(), 16 * sizeof(float));
}

// ============================================================================
// Scale Tests
// ============================================================================

TEST(Scale_Construction, Identity_ProducesIdentityMatrix)
{
    auto scale = Scale::identity();
    
    glm::mat4 expected = glm::mat4(1.0f);
    EXPECT_TRUE(matrices_equal(scale.matrix(), expected));
}

TEST(Scale_Construction, Uniform_ProducesUniformScale)
{
    auto scale = Scale::uniform(2.0f);

    glm::mat4 expected = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
    EXPECT_TRUE(matrices_equal(scale.matrix(), expected));
}

TEST(Scale_Construction, NonUniform_MatchesGLM)
{
    Scale scale(2.0f, 3.0f, 4.0f);

    glm::mat4 expected = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f));
    EXPECT_TRUE(matrices_equal(scale.matrix(), expected));
}

TEST(Scale_Construction, FromVector_MatchesGLM)
{
    glm::vec3 factors(0.5f, 1.5f, 2.0f);
    Scale scale(factors);

    glm::mat4 expected = glm::scale(glm::mat4(1.0f), factors);
    EXPECT_TRUE(matrices_equal(scale.matrix(), expected));
}

// ============================================================================
// Model Tests
// ============================================================================

TEST(Model_Construction, Identity_ProducesIdentityMatrix)
{
    auto model = Model::identity();
    
    glm::mat4 expected = glm::mat4(1.0f);
    EXPECT_TRUE(matrices_equal(model.matrix(), expected));
}

TEST(Model_Composition, MultiplyWithRotation_CombinesCorrectly)
{
    auto model = Model::identity();
    Rotation rot(glm::quarter_pi<float>(), Axis::Z());

    Model result = model * rot;

    glm::mat4 expected = glm::mat4(1.0f) * rot.matrix();
    EXPECT_TRUE(matrices_equal(result.matrix(), expected));
}

TEST(Model_Composition, MultiplyWithTranslation_CombinesCorrectly)
{
    auto model = Model::identity();
    Translation trans(5.0f, 10.0f, -3.0f);

    Model result = model * trans;

    glm::mat4 expected = glm::mat4(1.0f) * trans.matrix();
    EXPECT_TRUE(matrices_equal(result.matrix(), expected));
}

TEST(Model_Composition, MultiplyWithScale_CombinesCorrectly)
{
    auto model = Model::identity();
    Scale scale(2.0f, 2.0f, 2.0f);

    Model result = model * scale;

    glm::mat4 expected = glm::mat4(1.0f) * scale.matrix();
    EXPECT_TRUE(matrices_equal(result.matrix(), expected));
}

TEST(Model_Composition, ChainedTransformations_ApplyInCorrectOrder)
{
    Translation trans(1.0f, 2.0f, 3.0f);
    Rotation rot(glm::half_pi<float>(), Axis::Y());
    Scale scale(2.0f, 2.0f, 2.0f);

    // Build up: identity -> translate -> rotate -> scale
    Model model = Model::identity() * trans * rot * scale;

    // Expected: same operations in same order
    glm::mat4 expected = glm::mat4(1.0f);
    expected = expected * trans.matrix();
    expected = expected * rot.matrix();
    expected = expected * scale.matrix();

    EXPECT_TRUE(matrices_equal(model.matrix(), expected));
}

// ============================================================================
// View Tests
// ============================================================================

TEST(View_Construction, Identity_ProducesIdentityMatrix)
{
    auto view = View::identity();
    
    glm::mat4 expected = glm::mat4(1.0f);
    EXPECT_TRUE(matrices_equal(view.matrix(), expected));
}

TEST(View_Construction, LookAt_MatchesGLM)
{
    glm::vec3 eye(0.0f, 0.0f, 5.0f);
    glm::vec3 target(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    View view = View::look_at(eye, target, up);

    glm::mat4 expected = glm::lookAt(eye, target, up);
    EXPECT_TRUE(matrices_equal(view.matrix(), expected));
}

TEST(View_Construction, LookAt_DefaultUpIsYAxis)
{
    glm::vec3 eye(3.0f, 4.0f, 5.0f);
    glm::vec3 target(0.0f, 0.0f, 0.0f);

    View view = View::look_at(eye, target);

    glm::mat4 expected = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_TRUE(matrices_equal(view.matrix(), expected));
}

// ============================================================================
// Perspective Tests
// ============================================================================

TEST(Perspective_Construction, FromFov_MatchesGLM)
{
    float fov = glm::quarter_pi<float>(); // 45 degrees in radians
    float aspect = 16.0f / 9.0f;
    float near = 0.1f;
    float far = 100.0f;

    Perspective proj = Perspective::from_fov(fov, aspect, near, far);

    glm::mat4 expected = glm::perspective(fov, aspect, near, far);
    EXPECT_TRUE(matrices_equal(proj.matrix(), expected));
}

TEST(Perspective_Construction, FromFovDegrees_MatchesRadianVersion)
{
    float fov_degrees = 45.0f;
    float fov_radians = glm::radians(fov_degrees);
    float aspect = 4.0f / 3.0f;
    float near = 0.5f;
    float far = 1000.0f;

    Perspective proj_degrees = Perspective::from_fov_degrees(fov_degrees, aspect, near, far);
    Perspective proj_radians = Perspective::from_fov(fov_radians, aspect, near, far);

    EXPECT_TRUE(matrices_equal(proj_degrees.matrix(), proj_radians.matrix()));
}

TEST(Perspective_Construction, DifferentAspectRatios_ProduceDifferentMatrices)
{
    float fov = glm::half_pi<float>();
    float near = 0.1f;
    float far = 100.0f;

    Perspective proj1 = Perspective::from_fov(fov, 16.0f / 9.0f, near, far);
    Perspective proj2 = Perspective::from_fov(fov, 4.0f / 3.0f, near, far);

    EXPECT_FALSE(matrices_equal(proj1.matrix(), proj2.matrix()));
}

// ============================================================================
// Orthographic Tests
// ============================================================================

TEST(Orthographic_Construction, FromBounds_MatchesGLM)
{
    float left = -10.0f;
    float right = 10.0f;
    float bottom = -10.0f;
    float top = 10.0f;
    float near = 0.1f;
    float far = 100.0f;

    Orthographic proj = Orthographic::from_bounds(left, right, bottom, top, near, far);

    glm::mat4 expected = glm::ortho(left, right, bottom, top, near, far);
    EXPECT_TRUE(matrices_equal(proj.matrix(), expected));
}

TEST(Orthographic_Construction, AsymmetricBounds_MatchesGLM)
{
    Orthographic proj = Orthographic::from_bounds(-5.0f, 15.0f, -8.0f, 12.0f, 1.0f, 50.0f);

    glm::mat4 expected = glm::ortho(-5.0f, 15.0f, -8.0f, 12.0f, 1.0f, 50.0f);
    EXPECT_TRUE(matrices_equal(proj.matrix(), expected));
}

// ============================================================================
// Type-Safe Composition Tests
// ============================================================================

TEST(TypeSafeComposition, RotationTimesRotation_ProducesModel)
{
    Rotation r1(glm::quarter_pi<float>(), Axis::Y());
    Rotation r2(glm::quarter_pi<float>(), Axis::X());

    Model result = r1 * r2;

    glm::mat4 expected = r1.matrix() * r2.matrix();
    EXPECT_TRUE(matrices_equal(result.matrix(), expected));
}

TEST(TypeSafeComposition, TranslationTimesRotation_ProducesModel)
{
    Translation trans(1.0f, 2.0f, 3.0f);
    Rotation rot(glm::half_pi<float>(), Axis::Z());

    Model result = trans * rot;

    glm::mat4 expected = trans.matrix() * rot.matrix();
    EXPECT_TRUE(matrices_equal(result.matrix(), expected));
}

TEST(TypeSafeComposition, ViewTimesModel_ProducesModelView)
{
    View view = View::look_at(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0));
    Model model = Model::identity();

    ModelView result = view * model;

    glm::mat4 expected = view.matrix() * model.matrix();
    EXPECT_TRUE(matrices_equal(result.matrix(), expected));
}

TEST(TypeSafeComposition, ViewTimesRotation_ProducesModelView)
{
    View view = View::look_at(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0));
    Rotation rot(glm::quarter_pi<float>(), Axis::Y());

    ModelView result = view * rot;

    glm::mat4 expected = view.matrix() * rot.matrix();
    EXPECT_TRUE(matrices_equal(result.matrix(), expected));
}

TEST(TypeSafeComposition, PerspectiveTimesView_ProducesModelViewProjection)
{
    Perspective proj = Perspective::from_fov_degrees(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    View view = View::look_at(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0));

    ModelViewProjection result = proj * view;

    glm::mat4 expected = proj.matrix() * view.matrix();
    EXPECT_TRUE(matrices_equal(result.matrix(), expected));
}

TEST(TypeSafeComposition, PerspectiveTimesModelView_ProducesModelViewProjection)
{
    Perspective proj = Perspective::from_fov_degrees(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    View view = View::look_at(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0));
    Model model = Model::identity();
    ModelView mv = view * model;

    ModelViewProjection result = proj * mv;

    glm::mat4 expected = proj.matrix() * mv.matrix();
    EXPECT_TRUE(matrices_equal(result.matrix(), expected));
}

TEST(TypeSafeComposition, OrthographicTimesView_ProducesModelViewProjection)
{
    Orthographic proj = Orthographic::from_bounds(-10, 10, -10, 10, 0.1f, 100.0f);
    View view = View::look_at(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0));

    ModelViewProjection result = proj * view;

    glm::mat4 expected = proj.matrix() * view.matrix();
    EXPECT_TRUE(matrices_equal(result.matrix(), expected));
}

TEST(TypeSafeComposition, FullMVPChain_ProducesCorrectResult)
{
    // Build MVP the type-safe way
    Rotation model_rot(glm::quarter_pi<float>(), Axis::Y());
    View view = View::look_at(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0));
    Perspective proj = Perspective::from_fov_degrees(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);

    ModelViewProjection mvp = proj * view * model_rot;

    // Build MVP the GLM way
    glm::mat4 m = glm::rotate(glm::mat4(1.0f), glm::quarter_pi<float>(), glm::vec3(0, 1, 0));
    glm::mat4 v = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 p = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    glm::mat4 expected = p * v * m;

    EXPECT_TRUE(matrices_equal(mvp.matrix(), expected));
}

// ============================================================================
// Matrix Accessor Tests
// ============================================================================

TEST(MatrixAccessor, MatrixMethod_ReturnsConstReference)
{
    Rotation rot(glm::half_pi<float>(), Axis::Y());
    
    const glm::mat4& mat = rot.matrix();
    EXPECT_EQ(&mat, &rot.matrix()); // Same address (reference)
}

TEST(MatrixAccessor, ImplicitConversion_ToGlmMat4)
{
    Rotation rot(glm::quarter_pi<float>(), Axis::Y());
    
    // Should be able to use in GLM functions directly
    glm::mat4 copy = rot; // Implicit conversion
    EXPECT_TRUE(matrices_equal(copy, rot.matrix()));
}

TEST(MatrixAccessor, DataMethod_ReturnsColumnMajorData)
{
    Translation trans(1.0f, 2.0f, 3.0f);
    
    const float* data = trans.data();
    EXPECT_NE(data, nullptr);

    // Check that translation is in the last column (column-major)
    // Column 3 (index 12, 13, 14) contains translation
    EXPECT_FLOAT_EQ(data[12], 1.0f);
    EXPECT_FLOAT_EQ(data[13], 2.0f);
    EXPECT_FLOAT_EQ(data[14], 3.0f);
}

TEST(MatrixAccessor, ToBytes_HasCorrectSize)
{
    ModelViewProjection mvp(glm::mat4(1.0f));
    
    auto bytes = mvp.to_bytes();
    EXPECT_EQ(bytes.size(), 64); // 16 floats * 4 bytes
}

TEST(MatrixAccessor, ToBytes_ContainsCorrectData)
{
    // Create simple identity matrix
    Translation trans(5.0f, 10.0f, 15.0f);
    
    auto bytes = trans.to_bytes();
    const float* float_data = reinterpret_cast<const float*>(bytes.data());

    // Check translation components (column 3)
    EXPECT_FLOAT_EQ(float_data[12], 5.0f);
    EXPECT_FLOAT_EQ(float_data[13], 10.0f);
    EXPECT_FLOAT_EQ(float_data[14], 15.0f);
}

// ============================================================================
// Copy and Move Semantics Tests
// ============================================================================

TEST(CopySemantics, Rotation_CanBeCopied)
{
    Rotation original(glm::quarter_pi<float>(), Axis::Y());
    Rotation copy = original;

    EXPECT_TRUE(matrices_equal(original.matrix(), copy.matrix()));
}

TEST(CopySemantics, Model_CanBeCopied)
{
    Model original = Model::identity();
    Model copy = original;

    EXPECT_TRUE(matrices_equal(original.matrix(), copy.matrix()));
}

TEST(MoveSemantics, Rotation_CanBeMoved)
{
    Rotation original(glm::quarter_pi<float>(), Axis::Y());
    glm::mat4 original_matrix = original.matrix();
    
    Rotation moved = std::move(original);

    EXPECT_TRUE(matrices_equal(moved.matrix(), original_matrix));
}

TEST(MoveSemantics, ModelViewProjection_CanBeMoved)
{
    ModelViewProjection original(glm::mat4(1.0f));
    glm::mat4 original_matrix = original.matrix();
    
    ModelViewProjection moved = std::move(original);

    EXPECT_TRUE(matrices_equal(moved.matrix(), original_matrix));
}

// ============================================================================
// Real-World Usage Pattern Tests
// ============================================================================

TEST(UsagePattern, SpinningCube_ProducesValidMVP)
{
    // Simulate spinning cube transformation
    float angle = glm::radians(45.0f);
    Rotation model_rotation(angle, Axis::Y());
    
    View view = View::look_at(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    Perspective projection = Perspective::from_fov_degrees(
        45.0f,
        800.0f / 600.0f,
        0.1f, 100.0f
    );
    
    ModelViewProjection mvp = projection * view * model_rotation;
    
    // Verify MVP is valid (non-zero, finite)
    const float* data = mvp.data();
    bool has_nonzero = false;
    bool all_finite = true;
    
    for (int i = 0; i < 16; ++i)
    {
        if (data[i] != 0.0f) has_nonzero = true;
        if (!std::isfinite(data[i])) all_finite = false;
    }
    
    EXPECT_TRUE(has_nonzero);
    EXPECT_TRUE(all_finite);
}

TEST(UsagePattern, ComplexModelTransform_CombinesTRS)
{
    // Translate, Rotate, Scale pattern
    Translation trans(5.0f, 0.0f, 0.0f);
    Rotation rot(glm::quarter_pi<float>(), Axis::Z());
    Scale scale(2.0f, 2.0f, 2.0f);
    
    // Build model: T * R * S
    Model model = Model::identity() * trans * rot * scale;
    
    // Compare with GLM
    glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(5, 0, 0));
    glm::mat4 r = glm::rotate(glm::mat4(1.0f), glm::quarter_pi<float>(), glm::vec3(0, 0, 1));
    glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::vec3(2, 2, 2));
    glm::mat4 expected = t * r * s;
    
    EXPECT_TRUE(matrices_equal(model.matrix(), expected));
}

TEST(UsagePattern, OrthographicUI_ProducesValidProjection)
{
    // Typical orthographic projection for UI
    float width = 1920.0f;
    float height = 1080.0f;
    
    Orthographic proj = Orthographic::from_bounds(
        0.0f, width,
        0.0f, height,
        -1.0f, 1.0f
    );
    
    View view = View::identity();
    ModelViewProjection mvp = proj * view * Model::identity();
    
    // Verify it's a valid matrix
    const float* data = mvp.data();
    bool all_finite = true;
    for (int i = 0; i < 16; ++i)
    {
        if (!std::isfinite(data[i])) all_finite = false;
    }
    
    EXPECT_TRUE(all_finite);
}
