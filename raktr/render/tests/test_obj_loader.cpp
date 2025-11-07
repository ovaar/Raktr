/*!
 * @file test_obj_loader.cpp
 * @brief Unit tests for WaveFront OBJ loader with normals and UVs.
 */

#include "io/obj_loader.h"
#include <gtest/gtest.h>
#include <sstream>

namespace raktr::render::test
{

    TEST(ObjLoader_ParsePositions, ValidObjWithPositionsOnly_ReturnsSuccess)
    {
        // Arrange
        std::stringstream obj_data(R"(
v 0.0 1.0 0.0
v -1.0 -1.0 0.0
v 1.0 -1.0 0.0
f 1 2 3
)");

        // Act
        auto result = io::load_obj(obj_data);

        // Assert
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->positions.size(), 3);
        EXPECT_EQ(result->indices.size(), 3);

        // Verify first vertex
        EXPECT_FLOAT_EQ(result->positions[0].x, 0.0f);
        EXPECT_FLOAT_EQ(result->positions[0].y, 1.0f);
        EXPECT_FLOAT_EQ(result->positions[0].z, 0.0f);
    }

    TEST(ObjLoader_ParseNormals, ValidObjWithNormals_ReturnsSuccess)
    {
        // Arrange
        std::stringstream obj_data(R"(
v 0.0 1.0 0.0
v -1.0 -1.0 0.0
v 1.0 -1.0 0.0
vn 0.0 0.0 1.0
vn 0.0 0.0 1.0
vn 0.0 0.0 1.0
f 1//1 2//2 3//3
)");

        // Act
        auto result = io::load_obj(obj_data);

        // Assert
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->positions.size(), 3);
        EXPECT_EQ(result->normals.size(), 3);
        EXPECT_EQ(result->indices.size(), 3);

        // Verify normals
        EXPECT_FLOAT_EQ(result->normals[0].x, 0.0f);
        EXPECT_FLOAT_EQ(result->normals[0].y, 0.0f);
        EXPECT_FLOAT_EQ(result->normals[0].z, 1.0f);
    }

    TEST(ObjLoader_ParseUVs, ValidObjWithTextureCoords_ReturnsSuccess)
    {
        // Arrange
        std::stringstream obj_data(R"(
v 0.0 1.0 0.0
v -1.0 -1.0 0.0
v 1.0 -1.0 0.0
vt 0.5 1.0
vt 0.0 0.0
vt 1.0 0.0
f 1/1 2/2 3/3
)");

        // Act
        auto result = io::load_obj(obj_data);

        // Assert
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->positions.size(), 3);
        EXPECT_EQ(result->uvs.size(), 3);

        // Verify UVs
        EXPECT_FLOAT_EQ(result->uvs[0].x, 0.5f);
        EXPECT_FLOAT_EQ(result->uvs[0].y, 1.0f);
    }

    TEST(ObjLoader_ParseFull, ObjWithPositionsUVsNormals_ReturnsSuccess)
    {
        // Arrange
        std::stringstream obj_data(R"(
v 0.0 1.0 0.0
v -1.0 -1.0 0.0
v 1.0 -1.0 0.0
vt 0.5 1.0
vt 0.0 0.0
vt 1.0 0.0
vn 0.0 0.0 1.0
vn 0.0 0.0 1.0
vn 0.0 0.0 1.0
f 1/1/1 2/2/2 3/3/3
)");

        // Act
        auto result = io::load_obj(obj_data);

        // Assert
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->positions.size(), 3);
        EXPECT_EQ(result->uvs.size(), 3);
        EXPECT_EQ(result->normals.size(), 3);
        EXPECT_EQ(result->indices.size(), 3);
    }

    TEST(ObjLoader_ParseSquare, SquareFromPlanWithNormals_ReturnsSuccess)
    {
        // Arrange - the actual square from plan.md
        std::stringstream obj_data(R"(
v 0.5773502691896258 3.5773502691896257 0.5773502691896258
v 0.5773502691896258 3.5773502691896257 -0.5773502691896258
v -0.5773502691896258 3.5773502691896257 -0.5773502691896258
v -0.5773502691896258 3.5773502691896257 0.5773502691896258
vn 0 1 0
vn 0 1 0
f 1//1 2//1 3//1
f 1//2 3//2 4//2
)");

        // Act
        auto result = io::load_obj(obj_data);

        // Assert
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->positions.size(), 6); // 2 triangles = 6 vertices (expanded)
        EXPECT_EQ(result->normals.size(), 6);
        EXPECT_EQ(result->indices.size(), 6);

        // Verify all normals point up (0, 1, 0)
        for (const auto& normal : result->normals)
        {
            EXPECT_FLOAT_EQ(normal.x, 0.0f);
            EXPECT_FLOAT_EQ(normal.y, 1.0f);
            EXPECT_FLOAT_EQ(normal.z, 0.0f);
        }
    }

    TEST(ObjLoader_ParseQuad, QuadTriangulation_ProducesTwoTriangles)
    {
        // Arrange - quad face that should be triangulated
        std::stringstream obj_data(R"(
v -1.0 -1.0 0.0
v 1.0 -1.0 0.0
v 1.0 1.0 0.0
v -1.0 1.0 0.0
f 1 2 3 4
)");

        // Act
        auto result = io::load_obj(obj_data);

        // Assert
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->indices.size(), 6); // Quad → 2 triangles = 6 indices
    }

    TEST(ObjLoader_ErrorHandling, EmptyStream_ReturnsEmptyMesh)
    {
        // Arrange
        std::stringstream obj_data("");

        // Act
        auto result = io::load_obj(obj_data);

        // Assert
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result->positions.empty());
        EXPECT_TRUE(result->indices.empty());
    }

    TEST(ObjLoader_ErrorHandling, InvalidFaceIndex_ReturnsError)
    {
        // Arrange
        std::stringstream obj_data(R"(
v 0.0 1.0 0.0
f 1 2 3
)");

        // Act
        auto result = io::load_obj(obj_data);

        // Assert - indices 2 and 3 are out of bounds
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), io::ObjError::InvalidFormat);
    }

} // namespace raktr::render::test
