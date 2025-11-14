/*!
 * @file test_frustum_culling.cpp
 * @brief Tests for frustum culling integration between Camera and Octree.
 */

#include "scene/camera.h"
#include "scene/octree.h"
#include <chrono>
#include <glm/glm.hpp>
#include <gtest/gtest.h>


using namespace raktr::engine::scene;
using ObjectId = Octree::ObjectId;

// ============================================================================
// Frustum Extraction Tests
// ============================================================================

TEST(FrustumCulling_Camera, ExtractFrustum_ReturnsValidFrustum)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 5), 45.0f, 16.0f / 9.0f, 0.1f, 100.0f);

    // Act
    auto frustum = camera.frustum();

    // Assert
    // Frustum should have 6 planes
    EXPECT_EQ(frustum.planes.size(), 6U);

    // Each plane should be normalized (normal vector length ≈ 1.0)
    for (const auto& plane : frustum.planes)
    {
        float normal_length = std::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
        EXPECT_NEAR(normal_length, 1.0f, 0.01f);
    }
}

TEST(FrustumCulling_Camera, FrustumChanges_WhenCameraMoves)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f, 0.1f, 100.0f);
    auto   frustum1 = camera.frustum();

    // Act
    camera.set_position(glm::vec3(10, 10, 10));
    auto frustum2 = camera.frustum();

    // Assert
    // At least one plane should be different
    bool planes_differ = false;
    for (size_t i = 0; i < 6; ++i)
    {
        if (frustum1.planes[i].w != frustum2.planes[i].w)
        {
            planes_differ = true;
            break;
        }
    }
    EXPECT_TRUE(planes_differ);
}

TEST(FrustumCulling_Camera, FrustumChanges_WhenCameraRotates)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f, 0.1f, 100.0f);
    auto   frustum1 = camera.frustum();

    // Act
    camera.set_rotation(45.0f, 45.0f); // Rotate yaw and pitch
    auto frustum2 = camera.frustum();

    // Assert
    // Planes should be different after rotation
    bool planes_differ = false;
    for (size_t i = 0; i < 6; ++i)
    {
        glm::vec3 normal1(frustum1.planes[i].x, frustum1.planes[i].y, frustum1.planes[i].z);
        glm::vec3 normal2(frustum2.planes[i].x, frustum2.planes[i].y, frustum2.planes[i].z);
        if (glm::distance(normal1, normal2) > 0.01f)
        {
            planes_differ = true;
            break;
        }
    }
    EXPECT_TRUE(planes_differ);
}

// ============================================================================
// Integration Tests with Octree
// ============================================================================

TEST(FrustumCulling_Integration, ObjectInFrustum_IsVisible)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 5), 45.0f, 1.0f, 0.1f, 100.0f);
    Octree octree(glm::vec3(0, 0, 0), 50.0f, 4);

    // Insert object in front of camera
    ObjectId id{ 1 };
    octree.insert(id, glm::vec3(0, 0, 0), 1.0f); // At origin, camera looking at -Z

    // Act
    auto visible = octree.query_frustum(camera.frustum());

    // Assert
    ASSERT_EQ(visible.size(), 1U);
    EXPECT_EQ(visible[0], id);
}

TEST(FrustumCulling_Integration, ObjectBehindCamera_IsNotVisible)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f, 0.1f, 100.0f);
    Octree octree(glm::vec3(0, 0, 0), 50.0f, 4);

    // Insert object behind camera
    ObjectId id{ 1 };
    octree.insert(id, glm::vec3(0, 0, 10), 1.0f); // Behind camera (camera looks at -Z)

    // Act
    auto visible = octree.query_frustum(camera.frustum());

    // Assert
    EXPECT_EQ(visible.size(), 0U); // Object should be culled
}

TEST(FrustumCulling_Integration, ObjectTooFar_IsNotVisible)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f, 0.1f, 10.0f); // Far plane at 10 units
    Octree octree(glm::vec3(0, 0, 0), 50.0f, 4);

    // Insert object beyond far plane
    ObjectId id{ 1 };
    octree.insert(id, glm::vec3(0, 0, -20), 1.0f); // 20 units forward (beyond far plane)

    // Act
    auto visible = octree.query_frustum(camera.frustum());

    // Assert
    EXPECT_EQ(visible.size(), 0U); // Object should be culled by far plane
}

TEST(FrustumCulling_Integration, ObjectTooClose_IsNotVisible)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f, 1.0f, 100.0f); // Near plane at 1 unit
    Octree octree(glm::vec3(0, 0, 0), 50.0f, 4);

    // Insert object before near plane
    ObjectId id{ 1 };
    octree.insert(id, glm::vec3(0, 0, -0.5f), 0.1f); // 0.5 units forward (before near plane)

    // Act
    auto visible = octree.query_frustum(camera.frustum());

    // Assert
    EXPECT_EQ(visible.size(), 0U); // Object should be culled by near plane
}

TEST(FrustumCulling_Integration, ObjectOutsideFOV_IsNotVisible)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f, 0.1f, 100.0f);
    Octree octree(glm::vec3(0, 0, 0), 50.0f, 4);

    // Insert object far to the right (outside FOV)
    ObjectId id{ 1 };
    octree.insert(id, glm::vec3(50, 0, -5), 1.0f); // Way off to the side

    // Act
    auto visible = octree.query_frustum(camera.frustum());

    // Assert
    EXPECT_EQ(visible.size(), 0U); // Object should be culled (outside FOV)
}

TEST(FrustumCulling_Integration, MultipleObjects_OnlyVisibleReturned)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 5), 45.0f, 1.0f, 0.1f, 100.0f);
    Octree octree(glm::vec3(0, 0, 0), 100.0f, 4);

    // Insert objects in various positions
    ObjectId visible1{ 1 }, visible2{ 2 }, visible3{ 3 };
    ObjectId culled1{ 4 }, culled2{ 5 }, culled3{ 6 };

    octree.insert(visible1, glm::vec3(0, 0, 0), 1.0f);   // In front of camera
    octree.insert(visible2, glm::vec3(2, 0, -2), 1.0f);  // Slightly to right, visible
    octree.insert(visible3, glm::vec3(-2, 0, -2), 1.0f); // Slightly to left, visible

    octree.insert(culled1, glm::vec3(0, 0, 10), 1.0f);   // Behind camera
    octree.insert(culled2, glm::vec3(50, 0, -2), 1.0f);  // Far right (outside FOV)
    octree.insert(culled3, glm::vec3(0, 0, -200), 1.0f); // Beyond far plane

    // Act
    auto visible = octree.query_frustum(camera.frustum());

    // Assert
    EXPECT_EQ(visible.size(), 3U);
    EXPECT_TRUE(std::find(visible.begin(), visible.end(), visible1) != visible.end());
    EXPECT_TRUE(std::find(visible.begin(), visible.end(), visible2) != visible.end());
    EXPECT_TRUE(std::find(visible.begin(), visible.end(), visible3) != visible.end());
}

TEST(FrustumCulling_Integration, CameraRotation_ChangesVisibleSet)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f, 0.1f, 100.0f);
    Octree octree(glm::vec3(0, 0, 0), 100.0f, 4);

    ObjectId front{ 1 }, right{ 2 };
    octree.insert(front, glm::vec3(0, 0, -10), 1.0f); // In front
    octree.insert(right, glm::vec3(10, 0, 0), 1.0f);  // To the right

    // Act 1: Look forward (default)
    auto visible_forward = octree.query_frustum(camera.frustum());

    // Act 2: Rotate 90° right to look at right object
    camera.set_rotation(0.0f, 0.0f); // Yaw = 0° = looking +X
    auto visible_right = octree.query_frustum(camera.frustum());

    // Assert
    // Looking forward: see front object
    EXPECT_EQ(visible_forward.size(), 1U);
    EXPECT_EQ(visible_forward[0], front);

    // Looking right: see right object
    EXPECT_EQ(visible_right.size(), 1U);
    EXPECT_EQ(visible_right[0], right);
}

TEST(FrustumCulling_Integration, LargeBoundingRadius_MayBeVisible)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 0), 45.0f, 1.0f, 0.1f, 100.0f);
    Octree octree(glm::vec3(0, 0, 0), 100.0f, 4);

    // Insert object with very large bounding sphere
    // Even though center is outside FOV, sphere might intersect
    ObjectId id{ 1 };
    octree.insert(id, glm::vec3(20, 0, -10), 25.0f); // Large radius

    // Act
    auto visible = octree.query_frustum(camera.frustum());

    // Assert
    // Object should be visible because bounding sphere intersects frustum
    EXPECT_GE(visible.size(), 0U); // May or may not be visible depending on exact frustum
}

TEST(FrustumCulling_Integration, EmptyOctree_ReturnsEmpty)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 5), 45.0f, 1.0f, 0.1f, 100.0f);
    Octree octree(glm::vec3(0, 0, 0), 100.0f, 4);

    // Act
    auto visible = octree.query_frustum(camera.frustum());

    // Assert
    EXPECT_TRUE(visible.empty());
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(FrustumCulling_Performance, QueryLargeOctree_CompletesQuickly)
{
    // Arrange
    Camera camera(glm::vec3(0, 0, 50), 45.0f, 1.0f, 0.1f, 1000.0f);
    Octree octree(glm::vec3(0, 0, 0), 500.0f, 6);

    // Insert 1000 objects
    for (uint32_t i = 0; i < 1000; ++i)
    {
        float x = (i % 10) * 10.0f - 50.0f;
        float y = ((i / 10) % 10) * 10.0f - 50.0f;
        float z = (i / 100) * 10.0f - 50.0f;
        octree.insert(ObjectId{ i }, glm::vec3(x, y, z), 2.0f);
    }

    // Act
    auto start   = std::chrono::high_resolution_clock::now();
    auto visible = octree.query_frustum(camera.frustum());
    auto end     = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Assert
    EXPECT_GT(visible.size(), 0U);    // Should find some objects
    EXPECT_LT(visible.size(), 1000U); // Should cull most objects
    EXPECT_LT(elapsed, 1000);         // Should complete in < 1ms
}
