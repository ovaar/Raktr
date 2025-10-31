/*!
 * @file test_wireframe_extraction.cpp
 * @brief Unit tests for wireframe edge extraction algorithm.
 */

#include <gtest/gtest.h>
#include "mesh/wireframe.h"
#include "io/obj_loader.h"
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <filesystem>

using namespace raktr::render::mesh;
using namespace raktr::render::io;

/*!
 * @brief Test fixture for wireframe extraction tests.
 */
class WireframeExtraction : public ::testing::Test
{
protected:
    /*!
     * @brief Helper to get the resources path for test files.
     */
    std::filesystem::path get_resources_path() const
    {
        // From build/render/tests/Debug to resources/models/primitives
        auto current = std::filesystem::current_path();
        
        // Navigate up from build/render/tests/Debug to repository root
        auto root = current;
        while (root.has_parent_path() && root.filename() != "Raktr") {
            root = root.parent_path();
        }
        
        auto resources = root / "resources" / "models" / "primitives";
        return resources;
    }

    /*!
     * @brief Helper to check if an edge exists in line indices.
     */
    bool has_edge(const std::vector<uint32_t>& line_indices, uint32_t v0, uint32_t v1)
    {
        // Normalize edge (min index first)
        if (v0 > v1) std::swap(v0, v1);
        
        // Check all line segments
        for (size_t i = 0; i + 1 < line_indices.size(); i += 2)
        {
            uint32_t a = line_indices[i];
            uint32_t b = line_indices[i + 1];
            if (a > b) std::swap(a, b);
            
            if (a == v0 && b == v1)
                return true;
        }
        return false;
    }
    
    /*!
     * @brief Helper to count unique edges in line indices.
     */
    size_t count_unique_edges(const std::vector<uint32_t>& line_indices)
    {
        std::unordered_set<uint64_t> edges;
        for (size_t i = 0; i + 1 < line_indices.size(); i += 2)
        {
            uint32_t v0 = line_indices[i];
            uint32_t v1 = line_indices[i + 1];
            if (v0 > v1) std::swap(v0, v1);
            
            uint64_t edge = (static_cast<uint64_t>(v0) << 32) | v1;
            edges.insert(edge);
        }
        return edges.size();
    }
};

TEST_F(WireframeExtraction, EmptyIndices_ReturnsEmptyLines)
{
    // Arrange
    std::vector<uint32_t> tri_indices;
    
    // Act
    auto line_indices = extract_wireframe_indices(tri_indices);
    
    // Assert
    EXPECT_TRUE(line_indices.empty());
}

TEST_F(WireframeExtraction, SingleTriangle_Returns3Edges)
{
    // Arrange
    std::vector<uint32_t> tri_indices = {0, 1, 2};
    
    // Act
    auto line_indices = extract_wireframe_indices(tri_indices);
    
    // Assert
    EXPECT_EQ(line_indices.size(), 6);  // 3 edges × 2 vertices
    EXPECT_EQ(count_unique_edges(line_indices), 3);
    
    // Check all three edges exist
    EXPECT_TRUE(has_edge(line_indices, 0, 1));
    EXPECT_TRUE(has_edge(line_indices, 1, 2));
    EXPECT_TRUE(has_edge(line_indices, 0, 2));
}

TEST_F(WireframeExtraction, TwoTrianglesSharedEdge_RemovesDuplicate)
{
    // Arrange
    //   1
    //  /|\
    // 0-2-3
    // Two triangles: (0,1,2) and (1,3,2)
    // Shared edge: (1,2)
    std::vector<uint32_t> tri_indices = {0, 1, 2,  1, 3, 2};
    
    // Act
    auto line_indices = extract_wireframe_indices(tri_indices);
    
    // Assert
    EXPECT_EQ(line_indices.size(), 10);  // 5 unique edges × 2 vertices
    EXPECT_EQ(count_unique_edges(line_indices), 5);
    
    // Check all five unique edges exist
    EXPECT_TRUE(has_edge(line_indices, 0, 1));
    EXPECT_TRUE(has_edge(line_indices, 0, 2));
    EXPECT_TRUE(has_edge(line_indices, 1, 2));  // Shared edge
    EXPECT_TRUE(has_edge(line_indices, 1, 3));
    EXPECT_TRUE(has_edge(line_indices, 2, 3));
}

TEST_F(WireframeExtraction, Quad_Returns5Edges)
{
    // Arrange
    // Square quad as 2 triangles:
    // 3---2
    // |  /|
    // | / |
    // |/  |
    // 0---1
    std::vector<uint32_t> tri_indices = {0, 1, 2,  0, 2, 3};
    
    // Act
    auto line_indices = extract_wireframe_indices(tri_indices);
    
    // Assert
    EXPECT_EQ(line_indices.size(), 10);  // 5 edges × 2 vertices
    EXPECT_EQ(count_unique_edges(line_indices), 5);
    
    // Four perimeter edges + one diagonal
    EXPECT_TRUE(has_edge(line_indices, 0, 1));
    EXPECT_TRUE(has_edge(line_indices, 1, 2));
    EXPECT_TRUE(has_edge(line_indices, 2, 3));
    EXPECT_TRUE(has_edge(line_indices, 0, 3));
    EXPECT_TRUE(has_edge(line_indices, 0, 2));  // Diagonal
}

TEST_F(WireframeExtraction, MultipleDisjointTriangles_CorrectEdgeCount)
{
    // Arrange
    // Two separate triangles (no shared vertices)
    std::vector<uint32_t> tri_indices = {
        0, 1, 2,   // Triangle 1
        3, 4, 5    // Triangle 2
    };
    
    // Act
    auto line_indices = extract_wireframe_indices(tri_indices);
    
    // Assert
    EXPECT_EQ(line_indices.size(), 12);  // 6 edges × 2 vertices
    EXPECT_EQ(count_unique_edges(line_indices), 6);
}

TEST_F(WireframeExtraction, IncompleteTriangle_Ignored)
{
    // Arrange
    // Only 2 vertices provided (not enough for a triangle)
    std::vector<uint32_t> tri_indices = {0, 1};
    
    // Act
    auto line_indices = extract_wireframe_indices(tri_indices);
    
    // Assert
    EXPECT_TRUE(line_indices.empty());
}

TEST_F(WireframeExtraction, LoadCube_Returns12Edges)
{
    // Arrange
    auto path = get_resources_path() / "cube.obj";
    ASSERT_TRUE(std::filesystem::exists(path)) << "Cube primitive not found at: " << path;
    auto cube_result = load_obj_file(path.string());
    ASSERT_TRUE(cube_result.has_value()) << "Failed to load cube.obj";
    
    // Act
    auto line_indices = extract_wireframe_indices(cube_result->indices);
    
    // Assert
    // Cube has 12 triangles, each with 3 edges = 36 triangle edges
    // After deduplication: 12 outer edges + 24 face diagonal edges = 36 unique edges
    EXPECT_EQ(line_indices.size(), 72);  // 36 edges × 2 vertices
    EXPECT_EQ(count_unique_edges(line_indices), 36);
}

TEST_F(WireframeExtraction, LoadPyramid_Returns8Edges)
{
    // Arrange
    auto path = get_resources_path() / "pyramid.obj";
    ASSERT_TRUE(std::filesystem::exists(path)) << "Pyramid primitive not found at: " << path;
    auto pyramid_result = load_obj_file(path.string());
    ASSERT_TRUE(pyramid_result.has_value()) << "Failed to load pyramid.obj";
    
    // Act
    auto line_indices = extract_wireframe_indices(pyramid_result->indices);
    
    // Assert
    // Pyramid has 9 triangles, each with 3 edges = 27 triangle edges
    // With triangulation and deduplication: 18 unique edges
    EXPECT_EQ(line_indices.size(), 36);  // 18 edges × 2 vertices
    EXPECT_EQ(count_unique_edges(line_indices), 18);
}

TEST_F(WireframeExtraction, LoadSphere_CorrectEdgeCount)
{
    // Arrange
    auto path = get_resources_path() / "sphere.obj";
    ASSERT_TRUE(std::filesystem::exists(path)) << "Sphere primitive not found at: " << path;
    auto sphere_result = load_obj_file(path.string());
    ASSERT_TRUE(sphere_result.has_value()) << "Failed to load sphere.obj";
    
    // Act
    auto line_indices = extract_wireframe_indices(sphere_result->indices);
    
    // Assert
    // Sphere with 20 vertices and 36 triangles
    // Using Euler's formula for closed mesh: E ≈ 3V/2
    // Expected: ~30 edges for 20 vertices
    size_t unique_edges = count_unique_edges(line_indices);
    EXPECT_GT(unique_edges, 0);
    EXPECT_EQ(line_indices.size(), unique_edges * 2);  // Each edge = 2 vertices
    
    // Verify no duplicate edges in output
    EXPECT_EQ(count_unique_edges(line_indices), unique_edges);
}
