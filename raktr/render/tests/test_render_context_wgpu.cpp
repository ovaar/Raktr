/*!
 * @file test_render_context_wgpu.cpp
 * @brief Integration tests for RenderContext with WebGPU backend.
 */

#include <gtest/gtest.h>
#include "render_context.h"

using namespace raktr::render;

class RenderContextWgpuTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Tests require WebGPU-capable hardware
    }

    void TearDown() override
    {
        // Cleanup handled by RenderContext destructor
    }
};

TEST_F(RenderContextWgpuTest, CreateContext_WithWebGPU_Succeeds)
{
    RenderConfig config;
    config.backend = BackendType::WebGPU;
    config.enable_validation = false;

    auto context = create_render_context();
    ASSERT_NE(context, nullptr);
    
    auto result = context->initialize(config);
    ASSERT_TRUE(result.has_value()) << "Failed to initialize RenderContext with WebGPU backend";
    EXPECT_TRUE(context->is_initialized());
}

TEST_F(RenderContextWgpuTest, GetDevice_ReturnsValidDevice)
{
    RenderConfig config;
    config.backend = BackendType::WebGPU;
    config.enable_validation = false;

    auto context = create_render_context();
    ASSERT_NE(context, nullptr);
    
    auto result = context->initialize(config);
    ASSERT_TRUE(result.has_value());
    
    Device* device = context->device();
    EXPECT_NE(device, nullptr);
}

TEST_F(RenderContextWgpuTest, DeviceOperations_DoNotCrash)
{
    RenderConfig config;
    config.backend = BackendType::WebGPU;
    config.enable_validation = false;

    auto context = create_render_context();
    ASSERT_NE(context, nullptr);
    
    auto result = context->initialize(config);
    ASSERT_TRUE(result.has_value());
    
    Device* device = context->device();
    ASSERT_NE(device, nullptr);

    // Test basic device operations
    EXPECT_NO_THROW({
        device->clear();
        device->present();
    });
}

TEST_F(RenderContextWgpuTest, CreateBuffers_Succeeds)
{
    RenderConfig config;
    config.backend = BackendType::WebGPU;
    config.enable_validation = false;

    auto context = create_render_context();
    ASSERT_NE(context, nullptr);
    
    auto result = context->initialize(config);
    ASSERT_TRUE(result.has_value());
    
    Device* device = context->device();
    ASSERT_NE(device, nullptr);

    // Create vertex buffer
    float vertices[] = {0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f};
    auto vertex_data = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device->create_vertex_buffer(vertex_data);
    EXPECT_TRUE(vertex_buffer.has_value());

    // Create index buffer
    uint32_t indices[] = {0, 1, 2};
    auto index_data = std::as_bytes(std::span(indices));
    auto index_buffer = device->create_index_buffer(index_data);
    EXPECT_TRUE(index_buffer.has_value());
}

TEST_F(RenderContextWgpuTest, MultipleContexts_CanCoexist)
{
    RenderConfig config;
    config.backend = BackendType::WebGPU;
    config.enable_validation = false;

    auto context1 = create_render_context();
    ASSERT_NE(context1, nullptr);
    auto result1 = context1->initialize(config);
    ASSERT_TRUE(result1.has_value());

    auto context2 = create_render_context();
    ASSERT_NE(context2, nullptr);
    auto result2 = context2->initialize(config);
    ASSERT_TRUE(result2.has_value());

    // Both contexts should be valid
    EXPECT_NE(context1->device(), nullptr);
    EXPECT_NE(context2->device(), nullptr);
}
