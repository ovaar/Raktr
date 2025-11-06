/*!
 * @file test_wgpu_device.cpp
 * @brief Unit tests for WebGPU device wrapper.
 */

#include <gtest/gtest.h>
#include "backend/wgpu/wgpu_device.h"
#include "window/window.h"
#include "window/native_window.h"

#ifdef _WIN32
#include <windows.h>
#endif

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

TEST_F(WgpuDeviceTest, DrawIndexed_WithTriangle_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    
    // Create triangle vertices
    float vertices[] = {
        0.0f,  0.5f, 0.0f,  // Top
       -0.5f, -0.5f, 0.0f,  // Bottom left
        0.5f, -0.5f, 0.0f   // Bottom right
    };
    auto vertex_data = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device->create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value());
    
    // Create indices
    uint32_t indices[] = {0, 1, 2};
    auto index_data = std::as_bytes(std::span(indices));
    auto index_buffer = device->create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value());
    
    // Draw the triangle
    auto draw_result = device->draw_indexed(vertex_buffer.value(), index_buffer.value(), 3);
    EXPECT_TRUE(draw_result.has_value()) << "Failed to draw indexed triangle";
    
    // Present the result
    EXPECT_NO_THROW(device->present());
}

TEST_F(WgpuDeviceTest, DrawIndexed_MultipleFrames_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    
    // Create triangle
    float vertices[] = {0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f};
    auto vertex_data = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device->create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value());
    
    uint32_t indices[] = {0, 1, 2};
    auto index_data = std::as_bytes(std::span(indices));
    auto index_buffer = device->create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value());
    
    // Draw multiple frames
    for (int frame = 0; frame < 3; ++frame)
    {
        auto draw_result = device->draw_indexed(vertex_buffer.value(), index_buffer.value(), 3);
        EXPECT_TRUE(draw_result.has_value()) << "Failed on frame " << frame;
        device->present();
    }
}

TEST_F(WgpuDeviceTest, Resize_WithValidDimensions_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    
    // Resize to new dimensions
    auto resize_result = device->resize(1024, 768);
    EXPECT_TRUE(resize_result.has_value()) << "Failed to resize surface";
    
    // Verify rendering still works after resize
    float vertices[] = {0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f};
    auto vertex_data = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device->create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value());
    
    uint32_t indices[] = {0, 1, 2};
    auto index_data = std::as_bytes(std::span(indices));
    auto index_buffer = device->create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value());
    
    auto draw_result = device->draw_indexed(vertex_buffer.value(), index_buffer.value(), 3);
    EXPECT_TRUE(draw_result.has_value()) << "Failed to draw after resize";
    
    EXPECT_NO_THROW(device->present());
}

TEST_F(WgpuDeviceTest, Resize_WithZeroDimensions_Fails)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    
    // Attempt to resize to zero width
    auto resize_result1 = device->resize(0, 768);
    EXPECT_FALSE(resize_result1.has_value());
    
    // Attempt to resize to zero height
    auto resize_result2 = device->resize(1024, 0);
    EXPECT_FALSE(resize_result2.has_value());
    
    // Attempt to resize to zero both
    auto resize_result3 = device->resize(0, 0);
    EXPECT_FALSE(resize_result3.has_value());
}

TEST_F(WgpuDeviceTest, Resize_MultipleTimes_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());
    
    auto& device = result.value();
    
    // Resize multiple times
    EXPECT_TRUE(device->resize(640, 480).has_value());
    EXPECT_TRUE(device->resize(1920, 1080).has_value());
    EXPECT_TRUE(device->resize(1280, 720).has_value());
    
    // Verify rendering still works
    device->clear();
    EXPECT_NO_THROW(device->present());
}

// Integration test with NativeWindow to test resize callback
class WgpuNativeWindowTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
#ifdef _WIN32
        _test_handle = GetDesktopWindow();
#else
        _test_handle = reinterpret_cast<void*>(0x12345678);
#endif
    }

    void* _test_handle = nullptr;
};

TEST_F(WgpuNativeWindowTest, ResizeCallback_UpdatesSurfaceDimensions)
{
    // Create NativeWindow
    auto window_result = create_window_from_native(_test_handle, 800, 600);
    ASSERT_TRUE(window_result.has_value());
    auto& window = window_result.value();
    
    // Create WgpuDevice
    auto device_result = WgpuDevice::create(window.get(), false);
    ASSERT_TRUE(device_result.has_value());
    auto& device = device_result.value();
    
    // Set up resize callback to automatically resize surface
    bool callback_invoked = false;
    window->set_resize_callback([&](uint32_t width, uint32_t height) {
        callback_invoked = true;
        auto resize_result = device->resize(width, height);
        EXPECT_TRUE(resize_result.has_value());
    });
    
    // Simulate window resize from external code
    auto* native_window = dynamic_cast<detail::NativeWindow*>(window.get());
    ASSERT_NE(native_window, nullptr);
    native_window->update_dimensions(1024, 768);
    
    // Verify callback was invoked
    EXPECT_TRUE(callback_invoked);
    
    // Verify rendering still works after resize
    device->clear();
    EXPECT_NO_THROW(device->present());
}

TEST_F(WgpuNativeWindowTest, ResizeCallback_MultipleResizes_AllSucceed)
{
    auto window_result = create_window_from_native(_test_handle, 800, 600);
    ASSERT_TRUE(window_result.has_value());
    auto& window = window_result.value();
    
    auto device_result = WgpuDevice::create(window.get(), false);
    ASSERT_TRUE(device_result.has_value());
    auto& device = device_result.value();
    
    int callback_count = 0;
    window->set_resize_callback([&](uint32_t width, uint32_t height) {
        callback_count++;
        auto resize_result = device->resize(width, height);
        EXPECT_TRUE(resize_result.has_value());
    });
    
    auto* native_window = dynamic_cast<detail::NativeWindow*>(window.get());
    ASSERT_NE(native_window, nullptr);
    
    // Simulate multiple resizes
    native_window->update_dimensions(640, 480);
    native_window->update_dimensions(1920, 1080);
    native_window->update_dimensions(1280, 720);
    
    EXPECT_EQ(callback_count, 3);
    
    // Verify rendering works after all resizes
    device->clear();
    EXPECT_NO_THROW(device->present());
}
