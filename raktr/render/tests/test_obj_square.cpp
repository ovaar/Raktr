/*!
 * @file test_obj_square.cpp
 * @brief Unit tests for uploading OBJ square mesh data.
 */

#include "render_context.h"
#include <array>
#include <gtest/gtest.h>

namespace raktr::render::test
{

    /*!
     * @brief Test uploading the specific OBJ square mesh from the plan.
     *
     * OBJ content:
     * v 0.5773502691896258 3.5773502691896257 0.5773502691896258
     * v 0.5773502691896258 3.5773502691896257 -0.5773502691896258
     * v -0.5773502691896258 3.5773502691896257 -0.5773502691896258
     * v -0.5773502691896258 3.5773502691896257 0.5773502691896258
     * vn 0 1 0
     * vn 0 1 0
     * f 1//1 2//1 3//1
     * f 1//2 3//2 4//2
     */
    TEST(OBJSquare, UploadSquareMesh_CreatesBuffersSuccessfully)
    {
        // Arrange
        auto ctx = create_render_context();
        ASSERT_TRUE(ctx->initialize({ .backend = BackendType::Fake }).has_value());

        // Vertices from OBJ (positions only for MVP)
        std::array<float, 12> vertices = {
            0.5773502691896258f, 3.5773502691896257f, 0.5773502691896258f, // v1
            0.5773502691896258f,
            3.5773502691896257f,
            -0.5773502691896258f, // v2
            -0.5773502691896258f,
            3.5773502691896257f,
            -0.5773502691896258f, // v3
            -0.5773502691896258f,
            3.5773502691896257f,
            0.5773502691896258f // v4
        };

        // Indices from faces (OBJ uses 1-based indexing, convert to 0-based)
        // f 1//1 2//1 3//1 → triangle (0, 1, 2)
        // f 1//2 3//2 4//2 → triangle (0, 2, 3)
        std::array<uint32_t, 6> indices = {
            0, 1, 2, // First triangle
            0,
            2,
            3 // Second triangle
        };

        // Act
        auto vb_result = ctx->device()->create_vertex_buffer(std::as_bytes(std::span{ vertices }));
        auto ib_result = ctx->device()->create_index_buffer(std::as_bytes(std::span{ indices }));

        // Assert
        ASSERT_TRUE(vb_result.has_value()) << vb_result.error().message();
        ASSERT_TRUE(ib_result.has_value()) << ib_result.error().message();

        EXPECT_TRUE(vb_result->is_valid());
        EXPECT_TRUE(ib_result->is_valid());
        EXPECT_EQ(vb_result->type(), BufferType::Vertex);
        EXPECT_EQ(ib_result->type(), BufferType::Index);
    }

    TEST(OBJSquare, DrawIndexed_WithSquareMesh_Succeeds)
    {
        // Arrange
        auto ctx = create_render_context();
        ASSERT_TRUE(ctx->initialize({ .backend = BackendType::Fake }).has_value());

        std::array<float, 12> vertices = {
            0.5773502691896258f, 3.5773502691896257f, 0.5773502691896258f, 0.5773502691896258f, 3.5773502691896257f, -0.5773502691896258f, -0.5773502691896258f, 3.5773502691896257f, -0.5773502691896258f, -0.5773502691896258f, 3.5773502691896257f, 0.5773502691896258f
        };

        std::array<uint32_t, 6> indices = { 0, 1, 2, 0, 2, 3 };

        auto vb = ctx->device()->create_vertex_buffer(std::as_bytes(std::span{ vertices }));
        auto ib = ctx->device()->create_index_buffer(std::as_bytes(std::span{ indices }));
        ASSERT_TRUE(vb.has_value());
        ASSERT_TRUE(ib.has_value());

        // Act
        auto draw_result = ctx->device()->draw_indexed(*vb, *ib, 6);

        // Assert
        EXPECT_TRUE(draw_result.has_value()) << draw_result.error().message();
    }

} // namespace raktr::render::test
