/*!
 * @file test_buffer_creation.cpp
 * @brief Unit tests for buffer creation and management.
 */

#include <gtest/gtest.h>
#include "render_context.h"
#include <vector>

namespace raktr::render::test
{

class BufferCreationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ctx = create_render_context();
        RenderConfig config{.backend = BackendType::Fake};
        auto result = ctx->initialize(config);
        ASSERT_TRUE(result.has_value()) << "Failed to initialize render context";
    }

    std::unique_ptr<RenderContext> ctx;
};

TEST_F(BufferCreationTest, CreateVertexBuffer_WithValidData_Succeeds)
{
    // Arrange
    std::vector<float> vertices = {
        0.5f,  0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
       -0.5f, -0.5f, 0.0f
    };
    auto data = std::as_bytes(std::span{vertices});

    // Act
    auto buffer_result = ctx->device()->create_vertex_buffer(data);

    // Assert
    ASSERT_TRUE(buffer_result.has_value()) << buffer_result.error().message();
    EXPECT_TRUE(buffer_result->is_valid());
    EXPECT_EQ(buffer_result->type(), BufferType::Vertex);
}

TEST_F(BufferCreationTest, CreateIndexBuffer_WithValidData_Succeeds)
{
    // Arrange
    std::vector<uint32_t> indices = {0, 1, 2};
    auto data = std::as_bytes(std::span{indices});

    // Act
    auto buffer_result = ctx->device()->create_index_buffer(data);

    // Assert
    ASSERT_TRUE(buffer_result.has_value()) << buffer_result.error().message();
    EXPECT_TRUE(buffer_result->is_valid());
    EXPECT_EQ(buffer_result->type(), BufferType::Index);
}

TEST_F(BufferCreationTest, CreateVertexBuffer_WithEmptyData_Fails)
{
    // Arrange
    std::vector<float> empty;
    auto data = std::as_bytes(std::span{empty});

    // Act
    auto buffer_result = ctx->device()->create_vertex_buffer(data);

    // Assert
    EXPECT_FALSE(buffer_result.has_value());
    EXPECT_EQ(buffer_result.error(), RenderError::BufferCreationFailed);
}

} // namespace raktr::render::test
