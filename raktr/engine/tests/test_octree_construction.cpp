/*!
 * @file test_octree_construction.cpp
 * @brief Tests for Octree construction and basic state.
 *
 * Tests basic Octree creation, bounds verification, and empty state.
 * Follows Triple-A (Arrange / Act / Assert) pattern.
 */

#include "scene/octree.h"
#include <glm/glm.hpp>
#include <gtest/gtest.h>


using namespace raktr::engine::scene;

// ============================================================================
// Construction Tests
// ============================================================================

/*!
 * @test Octree can be constructed with default parameters.
 */
TEST(OctreeConstruction, default_parameters_creates_empty_octree)
{
    // Arrange & Act
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Assert
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Octree can be constructed with custom center.
 */
TEST(OctreeConstruction, custom_center_is_accepted)
{
    // Arrange & Act
    Octree octree(glm::vec3(100.0f, 200.0f, 300.0f), 50.0f);

    // Assert
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Octree can be constructed with custom half-size.
 */
TEST(OctreeConstruction, custom_half_size_is_accepted)
{
    // Arrange & Act
    Octree octree(glm::vec3(0.0f), 1000.0f);

    // Assert
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Octree can be constructed with custom max depth.
 */
TEST(OctreeConstruction, custom_max_depth_is_accepted)
{
    // Arrange & Act
    Octree octree(glm::vec3(0.0f), 100.0f, 10);

    // Assert
    EXPECT_EQ(octree.size(), 0);
}

/*!
 * @test Octree can be constructed with custom max objects per node.
 */
TEST(OctreeConstruction, custom_max_objects_per_node_is_accepted)
{
    // Arrange & Act
    Octree octree(glm::vec3(0.0f), 100.0f, 8, 16);

    // Assert
    EXPECT_EQ(octree.size(), 0);
}

// ============================================================================
// Empty State Tests
// ============================================================================

/*!
 * @test Empty octree has size zero.
 */
TEST(OctreeEmptyState, size_returns_zero_for_empty_octree)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    size_t size = octree.size();

    // Assert
    EXPECT_EQ(size, 0);
}

/*!
 * @test Empty octree stats show zero nodes and objects.
 */
TEST(OctreeEmptyState, stats_show_zero_for_empty_octree)
{
    // Arrange
    Octree octree(glm::vec3(0.0f), 100.0f);

    // Act
    auto stats = octree.stats();

    // Assert
    EXPECT_EQ(stats.total_nodes, 1); // Root node always exists
    EXPECT_EQ(stats.leaf_nodes, 1);  // Root is a leaf initially
    EXPECT_EQ(stats.total_objects, 0);
    EXPECT_EQ(stats.max_depth_used, 0); // No subdivision yet
    EXPECT_EQ(stats.max_objects_in_node, 0);
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

/*!
 * @test Octree can be move-constructed.
 */
TEST(OctreeMoveSemantic, move_constructor_transfers_ownership)
{
    // Arrange
    Octree octree1(glm::vec3(0.0f), 100.0f);

    // Act
    Octree octree2(std::move(octree1));

    // Assert
    EXPECT_EQ(octree2.size(), 0);
    // octree1 is in moved-from state, don't access it
}

/*!
 * @test Octree can be move-assigned.
 */
TEST(OctreeMoveSemantic, move_assignment_transfers_ownership)
{
    // Arrange
    Octree octree1(glm::vec3(0.0f), 100.0f);
    Octree octree2(glm::vec3(10.0f), 50.0f);

    // Act
    octree2 = std::move(octree1);

    // Assert
    EXPECT_EQ(octree2.size(), 0);
    // octree1 is in moved-from state, don't access it
}
