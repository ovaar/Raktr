/*!
 * @file test_render_context.cpp
 * @brief Unit tests for RenderContext.
 */

#include <gtest/gtest.h>
#include "render_context.h"

namespace raktr::render::test
{

TEST(RenderContext, CreateContext_ReturnsValidContext)
{
    // Arrange & Act
    auto ctx = create_render_context();

    // Assert
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(ctx->is_initialized());
}

TEST(RenderContext, Initialize_WithFakeBackend_Succeeds)
{
    // Arrange
    auto ctx = create_render_context();
    RenderConfig config{
        .backend = BackendType::Fake,
        .enable_validation = false
    };

    // Act
    auto result = ctx->initialize(config);

    // Assert
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(ctx->is_initialized());
    EXPECT_NE(ctx->device(), nullptr);
}

TEST(RenderContext, Initialize_WithUnsupportedBackend_Fails)
{
    // Arrange
    auto ctx = create_render_context();
    RenderConfig config{
        .backend = static_cast<BackendType>(999),  // Invalid
    };

    // Act
    auto result = ctx->initialize(config);

    // Assert
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), RenderError::BackendNotSupported);
}

TEST(RenderContext, Device_BeforeInitialize_ReturnsNull)
{
    // Arrange
    auto ctx = create_render_context();

    // Act
    auto* dev = ctx->device();

    // Assert
    EXPECT_EQ(dev, nullptr);
}

TEST(RenderContext, Shutdown_AfterInitialize_ClearsState)
{
    // Arrange
    auto ctx = create_render_context();
    auto init_result = ctx->initialize({.backend = BackendType::Fake});
    ASSERT_TRUE(init_result.has_value());

    // Act
    ctx->shutdown();

    // Assert
    EXPECT_FALSE(ctx->is_initialized());
    EXPECT_EQ(ctx->device(), nullptr);
}

} // namespace raktr::render::test
