/*!
 * @file test_wgpu_device.cpp
 * @brief Unit tests for WebGPU device wrapper.
 */

#include <gtest/gtest.h>
#include "backend/wgpu/wgpu_device.h"
#include "window/window.h"

using namespace raktr::render;
using namespace raktr::render::backend;

class WgpuDeviceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        WindowConfig config;
        config.width = 800;
        config.height = 600;
        config.title = "WgpuDevice Test Window";
        config.resizable = false;
        config.fullscreen = false;

        auto window_result = create_window(config);
        ASSERT_TRUE(window_result.has_value()) << "Failed to create test window";
        _window = std::move(window_result.value());
    }

    void TearDown() override
    {
        _window.reset();
    }

    std::unique_ptr<Window> _window;
};

TEST_F(WgpuDeviceTest, Create_WithValidWindow_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value()) << "Failed to create WgpuDevice";
    
    auto& device = result.value();
    EXPECT_NE(device, nullptr);
}

TEST_F(WgpuDeviceTest, Create_WithNullWindow_Fails)
{
    auto result = WgpuDevice::create(nullptr, false);
    EXPECT_FALSE(result.has_value());
}

TEST_F(WgpuDeviceTest, WgpuDevice_HandlesAreValid)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    EXPECT_NE(device->wgpu_device(), nullptr);
    EXPECT_NE(device->wgpu_queue(), nullptr);
}

TEST_F(WgpuDeviceTest, Clear_DoesNotCrash)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    EXPECT_NO_THROW(device->clear());
}

TEST_F(WgpuDeviceTest, Present_DoesNotCrash)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    EXPECT_NO_THROW(device->present());
}

TEST_F(WgpuDeviceTest, CreateVertexBuffer_WithValidData_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    
    // Create simple vertex data
    float vertices[] = {0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f};
    auto data = std::as_bytes(std::span(vertices));
    
    auto buffer_result = device->create_vertex_buffer(data);
    EXPECT_TRUE(buffer_result.has_value());
}

TEST_F(WgpuDeviceTest, CreateIndexBuffer_WithValidData_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    
    // Create simple index data
    uint32_t indices[] = {0, 1, 2};
    auto data = std::as_bytes(std::span(indices));
    
    auto buffer_result = device->create_index_buffer(data);
    EXPECT_TRUE(buffer_result.has_value());
}

TEST_F(WgpuDeviceTest, ClearAndPresent_Sequence_DoesNotCrash)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    
    // Simulate typical frame loop
    EXPECT_NO_THROW({
        device->clear();
        device->present();
    });
}
