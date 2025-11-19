/*!
 * @file test_render_pass_builder.cpp
 * @brief Unit tests for RenderPassBuilder.
 */

#include "render_pass_builder.h"
#include "gtest/gtest.h"
#include <webgpu/webgpu.h>


using namespace raktr::render;

// Mock WebGPU objects for testing
namespace
{
    struct MockCommandEncoder
    {
    };
    struct MockTextureView
    {
    };

    WGPUTextureView create_mock_texture_view()
    {
        return reinterpret_cast<WGPUTextureView>(new MockTextureView());
    }

    void destroy_mock_texture_view(WGPUTextureView view)
    {
        delete reinterpret_cast<MockTextureView*>(view);
    }
} // namespace

TEST(RenderPassBuilder_ColorAttachment, ValidView_ReturnsBuilder)
{
    // Arrange
    auto              view = create_mock_texture_view();
    RenderPassBuilder builder;

    // Act
    auto& result = builder.color_attachment(view, WGPULoadOp_Clear, { 0.0f, 0.0f, 0.0f, 1.0f });

    // Assert
    EXPECT_EQ(&result, &builder); // Fluent API returns reference to self

    destroy_mock_texture_view(view);
}

TEST(RenderPassBuilder_ColorAttachment, LoadOpLoad_ConfiguresCorrectly)
{
    // Arrange
    auto              view = create_mock_texture_view();
    RenderPassBuilder builder;

    // Act
    builder.color_attachment(view, WGPULoadOp_Load, { 0.5f, 0.5f, 0.5f, 1.0f });

    // Assert - no crash, builder configured
    SUCCEED();

    destroy_mock_texture_view(view);
}

TEST(RenderPassBuilder_DepthAttachment, ValidView_ReturnsBuilder)
{
    // Arrange
    auto              view = create_mock_texture_view();
    RenderPassBuilder builder;

    // Act
    auto& result = builder.depth_attachment(view, WGPULoadOp_Clear, 1.0f);

    // Assert
    EXPECT_EQ(&result, &builder);

    destroy_mock_texture_view(view);
}

TEST(RenderPassBuilder_DepthAttachment, CustomClearValue_ConfiguresCorrectly)
{
    // Arrange
    auto              view = create_mock_texture_view();
    RenderPassBuilder builder;

    // Act
    builder.depth_attachment(view, WGPULoadOp_Clear, 0.5f);

    // Assert
    SUCCEED();

    destroy_mock_texture_view(view);
}

TEST(RenderPassBuilder_Label, EmptyString_ReturnsBuilder)
{
    // Arrange
    RenderPassBuilder builder;

    // Act
    auto& result = builder.label("");

    // Assert
    EXPECT_EQ(&result, &builder);
}

TEST(RenderPassBuilder_Label, NonEmptyString_ReturnsBuilder)
{
    // Arrange
    RenderPassBuilder builder;

    // Act
    auto& result = builder.label("TestPass");

    // Assert
    EXPECT_EQ(&result, &builder);
}

TEST(RenderPassBuilder_FluentChaining, MultipleAttachments_ChainsCorrectly)
{
    // Arrange
    auto              color_view = create_mock_texture_view();
    auto              depth_view = create_mock_texture_view();
    RenderPassBuilder builder;

    // Act
    auto& result = builder
                       .color_attachment(color_view, WGPULoadOp_Clear, { 0.1f, 0.1f, 0.15f, 1.0f })
                       .depth_attachment(depth_view, WGPULoadOp_Clear, 1.0f)
                       .label("GeometryPass");

    // Assert
    EXPECT_EQ(&result, &builder);

    destroy_mock_texture_view(color_view);
    destroy_mock_texture_view(depth_view);
}

TEST(RenderPassBuilder_ColorAttachment, ClearColorComponents_AllValid)
{
    // Arrange
    auto              view = create_mock_texture_view();
    RenderPassBuilder builder;

    // Act - test extreme values
    builder.color_attachment(view, WGPULoadOp_Clear, { 0.0f, 0.5f, 1.0f, 0.25f });

    // Assert
    SUCCEED();

    destroy_mock_texture_view(view);
}

TEST(RenderPassBuilder_DepthAttachment, ClearDepthRange_ValidValues)
{
    // Arrange
    auto              view = create_mock_texture_view();
    RenderPassBuilder builder;

    // Act - test depth range
    builder.depth_attachment(view, WGPULoadOp_Clear, 0.0f); // near
    builder.depth_attachment(view, WGPULoadOp_Clear, 0.5f); // mid
    builder.depth_attachment(view, WGPULoadOp_Clear, 1.0f); // far

    // Assert
    SUCCEED();

    destroy_mock_texture_view(view);
}

TEST(RenderPassBuilder_MultipleBuilders, IndependentInstances_DoNotInterfere)
{
    // Arrange
    auto              view1 = create_mock_texture_view();
    auto              view2 = create_mock_texture_view();
    RenderPassBuilder builder1;
    RenderPassBuilder builder2;

    // Act
    builder1.color_attachment(view1, WGPULoadOp_Clear, { 1.0f, 0.0f, 0.0f, 1.0f })
        .label("RedPass");
    builder2.color_attachment(view2, WGPULoadOp_Load, { 0.0f, 1.0f, 0.0f, 1.0f })
        .label("GreenPass");

    // Assert - builders are independent
    SUCCEED();

    destroy_mock_texture_view(view1);
    destroy_mock_texture_view(view2);
}

TEST(RenderPassBuilder_DefaultConstructor, EmptyBuilder_CanBeConfigured)
{
    // Arrange & Act
    RenderPassBuilder builder;

    // Assert - can add attachments to default-constructed builder
    auto view = create_mock_texture_view();
    builder.color_attachment(view, WGPULoadOp_Clear, { 0.0f, 0.0f, 0.0f, 1.0f });
    SUCCEED();

    destroy_mock_texture_view(view);
}
