/*!
 * @file test_octree_removal.cpp
 * @brief Tests for Octree object removal.
 *
 * Tests removal of objects, size updates, and edge cases.
 * Follows Triple-A (Arrange / Act / Assert) pattern.
 */

#include "scene/octree.h"
#include <glm/glm.hpp>
#include <gtest/gtest.h>


using namespace raktr::engine::scene;

// ============================================================================
// Basic Removal Tests
// ============================================================================

/*!
 * @test Remove existing object decreases size.
 */
TEST(OctreeRemoval, remove_existing_object_decreases_size)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(1, glm::vec3(0.0f, 0.0f, 0.0f));

    // Act
    bool removed = octree.remove(1);

    // Assert
    EXPECT_TRUE(removed);
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Remove non-existent object fails.
 */
TEST(OctreeRemoval, remove_non_existent_object_fails)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool removed = octree.remove(999);

    // Assert
    EXPECT_FALSE(removed);
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Remove from empty octree fails.
 */
TEST(OctreeRemoval, remove_from_empty_octree_fails)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool removed = octree.remove(1);

    // Assert
    EXPECT_FALSE(removed);
    EXPECT_EQ(octree.size(), 0);
}

// ============================================================================
// Multiple Removal Tests
// ============================================================================

/*!
 * @test Remove multiple objects one by one.
 */
TEST(OctreeRemoval, remove_multiple_objects_succeeds)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(1, glm::vec3(10.0f, 0.0f, 0.0f));
    octree.insert(2, glm::vec3(0.0f, 10.0f, 0.0f));
    octree.insert(3, glm::vec3(0.0f, 0.0f, 10.0f));

    // Act
    bool removed1 = octree.remove(1);
    bool removed2 = octree.remove(2);
    bool removed3 = octree.remove(3);

    // Assert
    EXPECT_TRUE(removed1);
    EXPECT_TRUE(removed2);
    EXPECT_TRUE(removed3);
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Remove same object twice fails second time.
 */
TEST(OctreeRemoval, remove_same_object_twice_fails_second_time)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(1, glm::vec3(0.0f, 0.0f, 0.0f));

    // Act
    bool removed1 = octree.remove(1);
    bool removed2 = octree.remove(1); // Try again

    // Assert
    EXPECT_TRUE(removed1);
    EXPECT_FALSE(removed2);
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Remove partial subset of objects.
 */
TEST(OctreeRemoval, remove_partial_subset_updates_size_correctly)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(1, glm::vec3(10.0f, 0.0f, 0.0f));
    octree.insert(2, glm::vec3(0.0f, 10.0f, 0.0f));
    octree.insert(3, glm::vec3(0.0f, 0.0f, 10.0f));

    // Act - Remove only object 2
    bool removed = octree.remove(2);

    // Assert
    EXPECT_TRUE(removed);
    EXPECT_EQ(octree.size(), 2); // Objects 1 and 3 remain
}

// ============================================================================
// Removal from Subdivided Tree
// ============================================================================

/*!
 * @test Remove from subdivided tree succeeds.
 */
TEST(OctreeRemoval, remove_from_subdivided_tree_succeeds)
{
    // Arrange - Trigger subdivision
    Octree octree(glm::vec3(0.0f), 100.0f, 8, 4);
    octree.insert(1, glm::vec3(10.0f, 10.0f, 10.0f));
    octree.insert(2, glm::vec3(-10.0f, 10.0f, 10.0f));
    octree.insert(3, glm::vec3(10.0f, -10.0f, 10.0f));
    octree.insert(4, glm::vec3(-10.0f, -10.0f, 10.0f));
    octree.insert(5, glm::vec3(10.0f, 10.0f, -10.0f));

    // Act - Remove object from subdivided tree
    bool removed = octree.remove(3);

    // Assert
    EXPECT_TRUE(removed);
    EXPECT_EQ(octree.size(), 4);
}

/*!
 * @test Remove all objects from subdivided tree.
 */
TEST(OctreeRemoval, remove_all_from_subdivided_tree_empties_octree)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f, 8, 4);
    for (uint64_t i = 1; i <= 10; ++i)
    {
        float offset = static_cast<float>(i);
        octree.insert(i, glm::vec3(offset, 0.0f, 0.0f));
    }

    // Act - Remove all
    for (uint64_t i = 1; i <= 10; ++i)
    {
        octree.remove(i);
    }

    // Assert
    EXPECT_EQ(octree.size(), 0);
    auto stats = octree.stats();
    EXPECT_EQ(stats.total_objects, 0);
}

// ============================================================================
// Clear Tests
// ============================================================================

/*!
 * @test Clear removes all objects.
 */
TEST(OctreeClear, clear_removes_all_objects)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(1, glm::vec3(10.0f, 0.0f, 0.0f));
    octree.insert(2, glm::vec3(0.0f, 10.0f, 0.0f));
    octree.insert(3, glm::vec3(0.0f, 0.0f, 10.0f));

    // Act
    octree.clear();

    // Assert
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Clear on empty octree is safe.
 */
TEST(OctreeClear, clear_empty_octree_is_safe)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    octree.clear();

    // Assert
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Clear collapses tree structure.
 */
TEST(OctreeClear, clear_collapses_tree_structure)
{
    // Arrange - Create subdivided tree
    Octree octree(glm::vec3(0.0f), 100.0f, 8, 4);
    for (uint64_t i = 1; i <= 10; ++i)
    {
        float offset = static_cast<float>(i);
        octree.insert(i, glm::vec3(offset, 0.0f, 0.0f));
    }

    // Act
    octree.clear();

    // Assert
    auto stats = octree.stats();
    EXPECT_EQ(stats.total_nodes, 1);    // Only root remains
    EXPECT_EQ(stats.leaf_nodes, 1);     // Root is a leaf
    EXPECT_EQ(stats.max_depth_used, 0); // No subdivision
    EXPECT_EQ(stats.total_objects, 0);
}

/*!
 * @test Can insert after clear.
 */
TEST(OctreeClear, can_insert_after_clear)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(1, glm::vec3(0.0f, 0.0f, 0.0f));
    octree.clear();

    // Act
    bool inserted = octree.insert(2, glm::vec3(10.0f, 0.0f, 0.0f));

    // Assert
    EXPECT_TRUE(inserted);
    EXPECT_EQ(octree.size(), 1);
}

// ============================================================================
// Update Tests (Remove + Reinsert)
// ============================================================================

/*!
 * @test Update existing object succeeds.
 */
TEST(OctreeUpdate, update_existing_object_succeeds)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(1, glm::vec3(10.0f, 0.0f, 0.0f));

    // Act
    bool updated = octree.update(1, glm::vec3(20.0f, 0.0f, 0.0f));

    // Assert
    EXPECT_TRUE(updated);
    EXPECT_EQ(octree.size(), 1); // Size unchanged
}

/*!
 * @test Update non-existent object fails.
 */
TEST(OctreeUpdate, update_non_existent_object_fails)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    bool updated = octree.update(999, glm::vec3(0.0f, 0.0f, 0.0f));

    // Assert
    EXPECT_FALSE(updated);
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Update to out-of-bounds position fails.
 */
TEST(OctreeUpdate, update_to_out_of_bounds_fails)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(1, glm::vec3(0.0f, 0.0f, 0.0f));

    // Act
    bool updated = octree.update(1, glm::vec3(101.0f, 0.0f, 0.0f));

    // Assert
    EXPECT_FALSE(updated);
    EXPECT_EQ(octree.size(), 1); // Object remains at old position
}

/*!
 * @test Update multiple objects.
 */
TEST(OctreeUpdate, update_multiple_objects_succeeds)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);
    octree.insert(1, glm::vec3(10.0f, 0.0f, 0.0f));
    octree.insert(2, glm::vec3(0.0f, 10.0f, 0.0f));

    // Act
    bool updated1 = octree.update(1, glm::vec3(-10.0f, 0.0f, 0.0f));
    bool updated2 = octree.update(2, glm::vec3(0.0f, -10.0f, 0.0f));

    // Assert
    EXPECT_TRUE(updated1);
    EXPECT_TRUE(updated2);
    EXPECT_EQ(octree.size(), 2);
}
