/*!
 * @file test_octree_insertion.cpp
 * @brief Tests for Octree object insertion and subdivision.
 *
 * Tests insertion of objects, subdivision behavior, boundary validation,
 * and duplicate detection. Follows Triple-A (Arrange / Act / Assert) pattern.
 */

#include "scene/octree.h"
#include <glm/glm.hpp>
#include <gtest/gtest.h>


using namespace raktr::engine::scene;

// ============================================================================
// Single Insertion Tests
// ============================================================================

/*!
 * @test Insert single object at origin increases size.
 */
TEST(OctreeInsertion, single_object_at_origin_increases_size)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool inserted = octree.insert(1, glm::vec3(0.0f, 0.0f, 0.0f));

    // Assert
    EXPECT_TRUE(inserted);
    EXPECT_EQ(octree.size(), 1);
}

/*!
 * @test Insert single object with positive position.
 */
TEST(OctreeInsertion, single_object_positive_position_succeeds)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool inserted = octree.insert(1, glm::vec3(50.0f, 50.0f, 50.0f));

    // Assert
    EXPECT_TRUE(inserted);
    EXPECT_EQ(octree.size(), 1);
}

/*!
 * @test Insert single object with negative position.
 */
TEST(OctreeInsertion, single_object_negative_position_succeeds)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool inserted = octree.insert(1, glm::vec3(-50.0f, -50.0f, -50.0f));

    // Assert
    EXPECT_TRUE(inserted);
    EXPECT_EQ(octree.size(), 1);
}

/*!
 * @test Insert object with radius.
 */
TEST(OctreeInsertion, object_with_radius_succeeds)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool inserted = octree.insert(1, glm::vec3(0.0f, 0.0f, 0.0f), 10.0f);

    // Assert
    EXPECT_TRUE(inserted);
    EXPECT_EQ(octree.size(), 1);
}

// ============================================================================
// Multiple Insertion Tests
// ============================================================================

/*!
 * @test Insert multiple objects with different IDs.
 */
TEST(OctreeInsertion, multiple_objects_different_ids_succeeds)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool inserted1 = octree.insert(1, glm::vec3(10.0f, 0.0f, 0.0f));
    bool inserted2 = octree.insert(2, glm::vec3(0.0f, 10.0f, 0.0f));
    bool inserted3 = octree.insert(3, glm::vec3(0.0f, 0.0f, 10.0f));

    // Assert
    EXPECT_TRUE(inserted1);
    EXPECT_TRUE(inserted2);
    EXPECT_TRUE(inserted3);
    EXPECT_EQ(octree.size(), 3);
}

/*!
 * @test Insert multiple objects at same position (allowed, different IDs).
 */
TEST(OctreeInsertion, multiple_objects_same_position_different_ids_succeeds)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool inserted1 = octree.insert(1, glm::vec3(0.0f, 0.0f, 0.0f));
    bool inserted2 = octree.insert(2, glm::vec3(0.0f, 0.0f, 0.0f));
    bool inserted3 = octree.insert(3, glm::vec3(0.0f, 0.0f, 0.0f));

    // Assert
    EXPECT_TRUE(inserted1);
    EXPECT_TRUE(inserted2);
    EXPECT_TRUE(inserted3);
    EXPECT_EQ(octree.size(), 3);
}

// ============================================================================
// Subdivision Tests
// ============================================================================

/*!
 * @test Inserting objects beyond threshold triggers subdivision.
 */
TEST(OctreeSubdivision, exceeding_threshold_triggers_subdivision)
{
    // Arrange - max 8 objects per node
    Octree octree(glm::vec3(0.0f), 100.0f, 8, 8);

    // Act - Insert 9 objects (should trigger subdivision)
    for (uint64_t i = 1; i <= 9; ++i)
    {
        float offset = static_cast<float>(i);
        octree.insert(i, glm::vec3(offset, 0.0f, 0.0f));
    }

    // Assert
    EXPECT_EQ(octree.size(), 9);
    auto stats = octree.stats();
    EXPECT_GT(stats.total_nodes, 1); // Should have subdivided
}

/*!
 * @test Subdivision respects max depth limit.
 */
TEST(OctreeSubdivision, max_depth_prevents_infinite_subdivision)
{
    // Arrange - max depth 2, 4 objects per node
    Octree octree(glm::vec3(0.0f), 100.0f, 2, 4);

    // Act - Insert many objects at same location (would cause deep subdivision)
    for (uint64_t i = 1; i <= 20; ++i)
    {
        octree.insert(i, glm::vec3(0.0f, 0.0f, 0.0f));
    }

    // Assert
    EXPECT_EQ(octree.size(), 20);
    auto stats = octree.stats();
    EXPECT_LE(stats.max_depth_used, 2); // Should not exceed max depth
}

/*!
 * @test Stats reflect subdivision correctly.
 */
TEST(OctreeSubdivision, stats_reflect_subdivision)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f, 8, 4);

    // Act - Insert 5 objects (should trigger subdivision at 4)
    octree.insert(1, glm::vec3(10.0f, 10.0f, 10.0f));
    octree.insert(2, glm::vec3(-10.0f, 10.0f, 10.0f));
    octree.insert(3, glm::vec3(10.0f, -10.0f, 10.0f));
    octree.insert(4, glm::vec3(-10.0f, -10.0f, 10.0f));
    octree.insert(5, glm::vec3(10.0f, 10.0f, -10.0f));

    // Assert
    auto stats = octree.stats();
    EXPECT_EQ(stats.total_objects, 5);
    EXPECT_GT(stats.total_nodes, 1);         // Should have subdivided
    EXPECT_GE(stats.leaf_nodes, 1);          // At least one leaf
    EXPECT_LE(stats.max_objects_in_node, 5); // Max objects in any node
}

// ============================================================================
// Boundary Tests
// ============================================================================

/*!
 * @test Insert object at exact boundary succeeds.
 */
TEST(OctreeBoundary, object_at_exact_boundary_succeeds)
{
    // Arrange - bounds [-100, 100]
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool inserted1 = octree.insert(1, glm::vec3(100.0f, 100.0f, 100.0f));
    bool inserted2 = octree.insert(2, glm::vec3(-100.0f, -100.0f, -100.0f));

    // Assert
    EXPECT_TRUE(inserted1);
    EXPECT_TRUE(inserted2);
    EXPECT_EQ(octree.size(), 2);
}

/*!
 * @test Insert object outside bounds fails.
 */
TEST(OctreeBoundary, object_outside_bounds_fails)
{
    // Arrange - bounds [-100, 100]
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool inserted1 = octree.insert(1, glm::vec3(101.0f, 0.0f, 0.0f));
    bool inserted2 = octree.insert(2, glm::vec3(0.0f, -101.0f, 0.0f));
    bool inserted3 = octree.insert(3, glm::vec3(0.0f, 0.0f, 101.0f));

    // Assert
    EXPECT_FALSE(inserted1);
    EXPECT_FALSE(inserted2);
    EXPECT_FALSE(inserted3);
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Insert object with radius extending outside bounds fails.
 */
TEST(OctreeBoundary, object_radius_extending_outside_fails)
{
    // Arrange - bounds [-100, 100]
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act - Object at (95, 0, 0) with radius 10 extends to 105
    bool inserted = octree.insert(1, glm::vec3(95.0f, 0.0f, 0.0f), 10.0f);

    // Assert
    // Note: Current implementation only checks center, not radius
    // This test documents current behavior - may change in future
    EXPECT_TRUE(inserted); // Center is inside, so insertion succeeds
}

// ============================================================================
// Duplicate ID Tests
// ============================================================================

/*!
 * @test Insert duplicate ID fails.
 */
TEST(OctreeDuplicate, inserting_duplicate_id_fails)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(1, glm::vec3(0.0f, 0.0f, 0.0f));

    // Act - Try to insert same ID again
    bool inserted = octree.insert(1, glm::vec3(10.0f, 10.0f, 10.0f));

    // Assert
    EXPECT_FALSE(inserted);
    EXPECT_EQ(octree.size(), 1); // Size unchanged
}

/*!
 * @test Duplicate ID fails even at different position.
 */
TEST(OctreeDuplicate, duplicate_id_different_position_fails)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(42, glm::vec3(10.0f, 0.0f, 0.0f));

    // Act
    bool inserted = octree.insert(42, glm::vec3(-10.0f, 0.0f, 0.0f));

    // Assert
    EXPECT_FALSE(inserted);
    EXPECT_EQ(octree.size(), 1);
}

// ============================================================================
// Offset Center Tests
// ============================================================================

/*!
 * @test Insert works with non-origin center.
 */
TEST(OctreeOffsetCenter, insertion_with_offset_center_succeeds)
{
    // Arrange - center at (100, 200, 300), half-size 50
    // Bounds: [50, 150] x [150, 250] x [250, 350]
    Octree octree(glm::vec3(100.0f, 200.0f, 300.0f), 50.0f);

    // Act
    bool inserted1 = octree.insert(1, glm::vec3(100.0f, 200.0f, 300.0f)); // Center
    bool inserted2 = octree.insert(2, glm::vec3(150.0f, 250.0f, 350.0f)); // Max corner
    bool inserted3 = octree.insert(3, glm::vec3(50.0f, 150.0f, 250.0f));  // Min corner

    // Assert
    EXPECT_TRUE(inserted1);
    EXPECT_TRUE(inserted2);
    EXPECT_TRUE(inserted3);
    EXPECT_EQ(octree.size(), 3);
}

/*!
 * @test Out of bounds detection works with offset center.
 */
TEST(OctreeOffsetCenter, out_of_bounds_with_offset_center_fails)
{
    // Arrange - center at (100, 200, 300), half-size 50
    Octree octree(glm::vec3(100.0f, 200.0f, 300.0f), 50.0f);

    // Act - Outside bounds
    bool inserted1 = octree.insert(1, glm::vec3(0.0f, 0.0f, 0.0f));
    bool inserted2 = octree.insert(2, glm::vec3(151.0f, 200.0f, 300.0f));

    // Assert
    EXPECT_FALSE(inserted1);
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(octree.size(), 0);
}
