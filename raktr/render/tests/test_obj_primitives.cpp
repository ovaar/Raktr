/*!
 * @file test_obj_primitives.cpp
 * @brief Integration tests for loading primitive OBJ files from resources.
 */

#include "io/obj_loader.h"
#include <filesystem>
#include <gtest/gtest.h>

namespace raktr::render::test
{

    class ObjPrimitivesTest : public ::testing::Test
    {
    protected:
        std::filesystem::path get_resources_path() const
        {
            // From build/render/tests/Debug to resources/models/primitives
            // build -> .. -> root, then resources/models/primitives
            auto current = std::filesystem::current_path();

            // Navigate up from build/render/tests/Debug
            auto root = current;
            while (root.has_parent_path() && root.filename() != "Raktr")
            {
                root = root.parent_path();
            }

            auto resources = root / "resources" / "models" / "primitives";
            return resources;
        }
    };

    TEST_F(ObjPrimitivesTest, LoadTriangle_FromFile_ReturnsValidMesh)
    {
        // Arrange
        auto path = get_resources_path() / "triangle.obj";
        ASSERT_TRUE(std::filesystem::exists(path)) << "Triangle primitive not found at: " << path;

        // Act
        auto result = io::load_obj_file(path.string());

        // Assert
        ASSERT_TRUE(result.has_value()) << "Failed to load triangle.obj";
        EXPECT_EQ(result->positions.size(), 3) << "Triangle should have 3 vertices";
        EXPECT_EQ(result->normals.size(), 3) << "Triangle should have 3 normals";
        EXPECT_EQ(result->uvs.size(), 3) << "Triangle should have 3 UVs";
        EXPECT_EQ(result->indices.size(), 3) << "Triangle should have 3 indices";
    }

    TEST_F(ObjPrimitivesTest, LoadPlane_FromFile_ReturnsValidMesh)
    {
        // Arrange
        auto path = get_resources_path() / "plane.obj";
        ASSERT_TRUE(std::filesystem::exists(path)) << "Plane primitive not found at: " << path;

        // Act
        auto result = io::load_obj_file(path.string());

        // Assert
        ASSERT_TRUE(result.has_value()) << "Failed to load plane.obj";
        EXPECT_EQ(result->positions.size(), 6) << "Plane should have 6 vertices (2 triangles)";
        EXPECT_EQ(result->indices.size(), 6) << "Plane should have 6 indices";

        // Verify all normals point up (0, 1, 0)
        for (const auto& normal : result->normals)
        {
            EXPECT_FLOAT_EQ(normal.x, 0.0f);
            EXPECT_FLOAT_EQ(normal.y, 1.0f);
            EXPECT_FLOAT_EQ(normal.z, 0.0f);
        }
    }

    TEST_F(ObjPrimitivesTest, LoadCube_FromFile_ReturnsValidMesh)
    {
        // Arrange
        auto path = get_resources_path() / "cube.obj";
        ASSERT_TRUE(std::filesystem::exists(path)) << "Cube primitive not found at: " << path;

        // Act
        auto result = io::load_obj_file(path.string());

        // Assert
        ASSERT_TRUE(result.has_value()) << "Failed to load cube.obj";
        EXPECT_EQ(result->positions.size(), 36) << "Cube should have 36 vertices (12 triangles)";
        EXPECT_EQ(result->normals.size(), 36) << "Cube should have 36 normals";
        EXPECT_EQ(result->uvs.size(), 36) << "Cube should have 36 UVs";
        EXPECT_EQ(result->indices.size(), 36) << "Cube should have 36 indices";
    }

    TEST_F(ObjPrimitivesTest, LoadPyramid_FromFile_ReturnsValidMesh)
    {
        // Arrange
        auto path = get_resources_path() / "pyramid.obj";
        ASSERT_TRUE(std::filesystem::exists(path)) << "Pyramid primitive not found at: " << path;

        // Act
        auto result = io::load_obj_file(path.string());

        // Assert
        ASSERT_TRUE(result.has_value()) << "Failed to load pyramid.obj";
        EXPECT_EQ(result->positions.size(), 18) << "Pyramid should have 18 vertices (6 triangles)";
        EXPECT_EQ(result->normals.size(), 18) << "Pyramid should have 18 normals";
        EXPECT_EQ(result->indices.size(), 18) << "Pyramid should have 18 indices";
    }

    TEST_F(ObjPrimitivesTest, LoadSphere_FromFile_ReturnsValidMesh)
    {
        // Arrange
        auto path = get_resources_path() / "sphere.obj";
        ASSERT_TRUE(std::filesystem::exists(path)) << "Sphere primitive not found at: " << path;

        // Act
        auto result = io::load_obj_file(path.string());

        // Assert
        ASSERT_TRUE(result.has_value()) << "Failed to load sphere.obj";
        EXPECT_GT(result->positions.size(), 0) << "Sphere should have vertices";
        EXPECT_GT(result->normals.size(), 0) << "Sphere should have normals";
        EXPECT_GT(result->indices.size(), 0) << "Sphere should have indices";
        EXPECT_EQ(result->indices.size() % 3, 0) << "Sphere indices should be multiple of 3";
    }

    TEST_F(ObjPrimitivesTest, LoadCube_VertexPositions_AreWithinBounds)
    {
        // Arrange
        auto path   = get_resources_path() / "cube.obj";
        auto result = io::load_obj_file(path.string());
        ASSERT_TRUE(result.has_value());

        // Act & Assert - all vertices should be within [-1, 1] cube
        for (const auto& pos : result->positions)
        {
            EXPECT_GE(pos.x, -1.0f) << "X coordinate out of bounds";
            EXPECT_LE(pos.x, 1.0f) << "X coordinate out of bounds";
            EXPECT_GE(pos.y, -1.0f) << "Y coordinate out of bounds";
            EXPECT_LE(pos.y, 1.0f) << "Y coordinate out of bounds";
            EXPECT_GE(pos.z, -1.0f) << "Z coordinate out of bounds";
            EXPECT_LE(pos.z, 1.0f) << "Z coordinate out of bounds";
        }
    }

    TEST_F(ObjPrimitivesTest, LoadPrimitives_AllNormalsAreNormalized)
    {
        // Arrange
        const std::vector<std::string> primitives = {
            "triangle.obj", "plane.obj", "cube.obj", "pyramid.obj", "sphere.obj"
        };

        for (const auto& primitive : primitives)
        {
            auto path = get_resources_path() / primitive;
            if (!std::filesystem::exists(path))
                continue;

            // Act
            auto result = io::load_obj_file(path.string());
            ASSERT_TRUE(result.has_value()) << "Failed to load " << primitive;

            // Assert - all normals should have length ~1.0
            for (const auto& normal : result->normals)
            {
                const float length = std::sqrt(normal.x * normal.x +
                                               normal.y * normal.y +
                                               normal.z * normal.z);
                EXPECT_NEAR(length, 1.0f, 0.1f)
                    << "Normal not normalized in " << primitive
                    << ": (" << normal.x << ", " << normal.y << ", " << normal.z << ")";
            }
        }
    }

    TEST_F(ObjPrimitivesTest, LoadPrimitives_UVsAreInValidRange)
    {
        // Arrange
        const std::vector<std::string> primitives = {
            "triangle.obj", "plane.obj", "cube.obj", "pyramid.obj"
        };

        for (const auto& primitive : primitives)
        {
            auto path = get_resources_path() / primitive;
            if (!std::filesystem::exists(path))
                continue;

            // Act
            auto result = io::load_obj_file(path.string());
            ASSERT_TRUE(result.has_value()) << "Failed to load " << primitive;

            // Assert - UVs should be in [0, 1] range
            for (const auto& uv : result->uvs)
            {
                EXPECT_GE(uv.x, 0.0f) << "UV.x out of range in " << primitive;
                EXPECT_LE(uv.x, 1.0f) << "UV.x out of range in " << primitive;
                EXPECT_GE(uv.y, 0.0f) << "UV.y out of range in " << primitive;
                EXPECT_LE(uv.y, 1.0f) << "UV.y out of range in " << primitive;
            }
        }
    }

} // namespace raktr::render::test
