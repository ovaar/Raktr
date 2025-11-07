/*!
 * @file test_fake_rasterization.cpp
 * @brief Unit tests for FakeDevice rasterization behavior.
 */

#include "render_context.h"
#include <array>
#include <gtest/gtest.h>

namespace raktr::render::test
{

    class FakeRasterizationTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            ctx = create_render_context();
            RenderConfig config{ .backend = BackendType::Fake };
            auto         result = ctx->initialize(config);
            ASSERT_TRUE(result.has_value()) << "Failed to initialize render context";
        }

        std::unique_ptr<RenderContext> ctx;
    };

    TEST_F(FakeRasterizationTest, DrawIndexed_TriangleInNDC_RasterizesPixels)
    {
        // Arrange - Create a triangle in NDC space that covers screen pixels
        std::array<float, 9> vertices = {
            0.0f, 0.5f, 0.0f, // Top center
            -0.5f,
            -0.5f,
            0.0f, // Bottom left
            0.5f,
            -0.5f,
            0.0f // Bottom right
        };
        std::array<uint32_t, 3> indices = { 0, 1, 2 };

        auto vb = ctx->device()->create_vertex_buffer(std::as_bytes(std::span{ vertices }));
        auto ib = ctx->device()->create_index_buffer(std::as_bytes(std::span{ indices }));
        ASSERT_TRUE(vb.has_value());
        ASSERT_TRUE(ib.has_value());

        // Act
        ctx->device()->clear();
        auto draw_result = ctx->device()->draw_indexed(*vb, *ib, 3);

        // Assert
        ASSERT_TRUE(draw_result.has_value()) << draw_result.error().message();

        // Note: We can't directly access FakeDevice methods from public API
        // This test verifies the draw operation succeeds
        // A more complete test would expose pixel readback through Device interface
    }

    TEST_F(FakeRasterizationTest, DrawIndexed_MultipleTriangles_Succeeds)
    {
        // Arrange - Two triangles forming a square (like OBJ test but in NDC)
        std::array<float, 12> vertices = {
            0.5f, 0.5f, 0.0f, // Top right
            0.5f,
            -0.5f,
            0.0f, // Bottom right
            -0.5f,
            -0.5f,
            0.0f, // Bottom left
            -0.5f,
            0.5f,
            0.0f // Top left
        };
        std::array<uint32_t, 6> indices = {
            0, 1, 2, // First triangle
            0,
            2,
            3 // Second triangle
        };

        auto vb = ctx->device()->create_vertex_buffer(std::as_bytes(std::span{ vertices }));
        auto ib = ctx->device()->create_index_buffer(std::as_bytes(std::span{ indices }));
        ASSERT_TRUE(vb.has_value());
        ASSERT_TRUE(ib.has_value());

        // Act
        ctx->device()->clear();
        auto draw_result = ctx->device()->draw_indexed(*vb, *ib, 6);

        // Assert
        EXPECT_TRUE(draw_result.has_value()) << draw_result.error().message();
    }

    TEST_F(FakeRasterizationTest, DrawIndexed_WithInvalidBuffer_Fails)
    {
        // Arrange - Create valid vertex buffer but use invalid index buffer
        std::array<float, 9> vertices = {
            0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f
        };

        auto vb = ctx->device()->create_vertex_buffer(std::as_bytes(std::span{ vertices }));
        ASSERT_TRUE(vb.has_value());

        Buffer invalid_ib(999, BufferType::Index); // Non-existent buffer

        // Act
        auto draw_result = ctx->device()->draw_indexed(*vb, invalid_ib, 3);

        // Assert
        EXPECT_FALSE(draw_result.has_value());
        EXPECT_EQ(draw_result.error(), RenderError::InvalidOperation);
    }

    TEST_F(FakeRasterizationTest, Clear_ResetsFramebuffer)
    {
        // Arrange
        std::array<float, 9> vertices = {
            0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f
        };
        std::array<uint32_t, 3> indices = { 0, 1, 2 };

        auto vb = ctx->device()->create_vertex_buffer(std::as_bytes(std::span{ vertices }));
        auto ib = ctx->device()->create_index_buffer(std::as_bytes(std::span{ indices }));
        ASSERT_TRUE(vb.has_value());
        ASSERT_TRUE(ib.has_value());

        // Draw something
        auto draw_result = ctx->device()->draw_indexed(*vb, *ib, 3);
        ASSERT_TRUE(draw_result.has_value());

        // Act - Clear should reset framebuffer
        ctx->device()->clear();

        // Assert - Just verify clear doesn't crash
        // Full validation would require pixel readback API
    }

} // namespace raktr::render::test
