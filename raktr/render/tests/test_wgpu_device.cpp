/*!
 * @file test_wgpu_device.cpp
 * @brief Unit tests for WebGPU device wrapper.
 */

#include "backend/wgpu/wgpu_device.h"
#include "window/native_window.h"
#include "window/window.h"
#include <gtest/gtest.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace raktr::render;
using namespace raktr::render::backend::wgpu;

class WgpuDeviceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        WindowConfig config;
        config.width      = 800;
        config.height     = 600;
        config.title      = "WgpuDevice Test Window";
        config.resizable  = false;
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
    EXPECT_NE(device.wgpu_device(), nullptr);
    EXPECT_NE(device.wgpu_queue(), nullptr);
}

TEST_F(WgpuDeviceTest, Clear_DoesNotCrash)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();
    EXPECT_NO_THROW(device.clear());
}

TEST_F(WgpuDeviceTest, Present_DoesNotCrash)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();
    EXPECT_NO_THROW(device.present());
}

TEST_F(WgpuDeviceTest, CreateVertexBuffer_WithValidData_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Create simple vertex data
    // clang-format off
    float vertices[] = {
        0.0f, 0.5f, 0.0f, 
        -0.5f, -0.5f, 0.0f, 
        0.5f, -0.5f, 0.0f 
    };
    // clang-format on
    auto data = std::as_bytes(std::span(vertices));

    auto buffer_result = device.create_vertex_buffer(data);
    EXPECT_TRUE(buffer_result.has_value());
}

TEST_F(WgpuDeviceTest, CreateIndexBuffer_WithValidData_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Create simple index data
    uint32_t indices[] = { 0, 1, 2 };
    auto     data      = std::as_bytes(std::span(indices));

    auto buffer_result = device.create_index_buffer(data);
    EXPECT_TRUE(buffer_result.has_value());
}

TEST_F(WgpuDeviceTest, ClearAndPresent_Sequence_DoesNotCrash)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Simulate typical frame loop
    EXPECT_NO_THROW({
        device.clear();
        device.present();
    });
}

TEST_F(WgpuDeviceTest, DrawIndexed_WithTriangle_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Create triangle vertices
    // clang-format off
    float vertices[] = {
        0.0f, 0.5f,   0.0f, // Top
        -0.5f, -0.5f, 0.0f, // Bottom left
        0.5f, -0.5f,  0.0f  // Bottom right
    };
    // clang-format on
    auto vertex_data   = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device.create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value());

    // Create indices
    uint32_t indices[]    = { 0, 1, 2 };
    auto     index_data   = std::as_bytes(std::span(indices));
    auto     index_buffer = device.create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value());

    // Draw the triangle
    auto draw_result = device.draw_indexed(vertex_buffer.value(), index_buffer.value(), 3);
    EXPECT_TRUE(draw_result.has_value()) << "Failed to draw indexed triangle";

    // Present the result
    EXPECT_NO_THROW(device.present());
}

TEST_F(WgpuDeviceTest, DrawIndexed_MultipleFrames_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Create triangle
    // clang-format off
    float vertices[] = {
        0.0f, 0.5f, 0.0f, 
        -0.5f, -0.5f, 0.0f, 
        0.5f, -0.5f, 0.0f
    };
    // clang-format on
    auto vertex_data   = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device.create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value());

    uint32_t indices[]    = { 0, 1, 2 };
    auto     index_data   = std::as_bytes(std::span(indices));
    auto     index_buffer = device.create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value());

    // Draw multiple frames
    for (int frame = 0; frame < 3; ++frame)
    {
        auto draw_result = device.draw_indexed(vertex_buffer.value(), index_buffer.value(), 3);
        EXPECT_TRUE(draw_result.has_value()) << "Failed on frame " << frame;
        device.present();
    }
}

TEST_F(WgpuDeviceTest, Resize_WithValidDimensions_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Resize to new dimensions
    auto resize_result = device.resize(1024, 768);
    EXPECT_TRUE(resize_result.has_value()) << "Failed to resize surface";

    // Verify rendering still works after resize
    // clang-format off
    float vertices[] = {
        0.0f, 0.5f, 0.0f, 
        -0.5f, -0.5f, 0.0f, 
        0.5f, -0.5f, 0.0f
    };
    // clang-format on
    auto vertex_data   = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device.create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value());

    uint32_t indices[]    = { 0, 1, 2 };
    auto     index_data   = std::as_bytes(std::span(indices));
    auto     index_buffer = device.create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value());

    auto draw_result = device.draw_indexed(vertex_buffer.value(), index_buffer.value(), 3);
    EXPECT_TRUE(draw_result.has_value()) << "Failed to draw after resize";

    EXPECT_NO_THROW(device.present());
}

TEST_F(WgpuDeviceTest, Resize_WithZeroDimensions_Fails)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Attempt to resize to zero width
    auto resize_result1 = device.resize(0, 768);
    EXPECT_FALSE(resize_result1.has_value());

    // Attempt to resize to zero height
    auto resize_result2 = device.resize(1024, 0);
    EXPECT_FALSE(resize_result2.has_value());

    // Attempt to resize to zero both
    auto resize_result3 = device.resize(0, 0);
    EXPECT_FALSE(resize_result3.has_value());
}

TEST_F(WgpuDeviceTest, Resize_MultipleTimes_Succeeds)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Resize multiple times
    EXPECT_TRUE(device.resize(640, 480).has_value());
    EXPECT_TRUE(device.resize(1920, 1080).has_value());
    EXPECT_TRUE(device.resize(1280, 720).has_value());

    // Verify rendering still works
    device.clear();
    EXPECT_NO_THROW(device.present());
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
    window->set_resize_callback([&](uint32_t width, uint32_t height)
                                {
                                    callback_invoked   = true;
                                    auto resize_result = device.resize(width, height);
                                    EXPECT_TRUE(resize_result.has_value());
                                });

    // Simulate window resize from external code
    auto* native_window = dynamic_cast<detail::NativeWindow*>(window.get());
    ASSERT_NE(native_window, nullptr);
    native_window->update_dimensions(1024, 768);

    // Verify callback was invoked
    EXPECT_TRUE(callback_invoked);

    // Verify rendering still works after resize
    device.clear();
    EXPECT_NO_THROW(device.present());
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
    window->set_resize_callback([&](uint32_t width, uint32_t height)
                                {
                                    callback_count++;
                                    auto resize_result = device.resize(width, height);
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
    device.clear();
    EXPECT_NO_THROW(device.present());
}

// ============================================================================
// Test: Aspect Ratio Support
// ============================================================================

TEST_F(WgpuDeviceTest, AspectRatio_DefaultIs16_9)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();
    EXPECT_EQ(device.aspect_ratio(), AspectRatio::Ratio_16_9);
}

TEST_F(WgpuDeviceTest, SetAspectRatio_UpdatesAspectRatio)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Test setting different aspect ratios
    device.set_aspect_ratio(AspectRatio::Ratio_4_3);
    EXPECT_EQ(device.aspect_ratio(), AspectRatio::Ratio_4_3);

    device.set_aspect_ratio(AspectRatio::Ratio_21_9);
    EXPECT_EQ(device.aspect_ratio(), AspectRatio::Ratio_21_9);

    device.set_aspect_ratio(AspectRatio::Auto);
    EXPECT_EQ(device.aspect_ratio(), AspectRatio::Auto);
}

TEST_F(WgpuDeviceTest, SetAspectRatio_RecalculatesViewport)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Get initial viewport (16:9 default with 800x600 window)
    // Note: 800x600 is 4:3, so 16:9 will be letterboxed
    auto initial_vp = device.viewport();

    // Change to 21:9 (ultrawide - even more letterboxed)
    device.set_aspect_ratio(AspectRatio::Ratio_21_9);
    auto new_vp = device.viewport();

    // 21:9 should produce a more narrow viewport (more letterboxing)
    EXPECT_EQ(new_vp.width, 800u);               // Width stays same
    EXPECT_LT(new_vp.height, initial_vp.height); // Height should be smaller
    EXPECT_GT(new_vp.y, initial_vp.y);           // More top/bottom padding
}

TEST_F(WgpuDeviceTest, Viewport_16_9_WithWideWindow_MatchesWindow)
{
    // Create 16:9 window
    WindowConfig config;
    config.width     = 1920;
    config.height    = 1080;
    config.resizable = false;

    auto window_result = create_window(config);
    ASSERT_TRUE(window_result.has_value());

    auto device_result = WgpuDevice::create(window_result.value().get(), false);
    ASSERT_TRUE(device_result.has_value());

    auto& device = device_result.value();
    auto  vp     = device.viewport();

    // Should use full window (exact match)
    EXPECT_EQ(vp.x, 0u);
    EXPECT_EQ(vp.y, 0u);
    EXPECT_EQ(vp.width, 1920u);
    EXPECT_EQ(vp.height, 1080u);
}

TEST_F(WgpuDeviceTest, Viewport_16_9_WithTallWindow_Letterboxes)
{
    // Create tall window (1920x1200 is 16:10, taller than 16:9)
    WindowConfig config;
    config.width     = 1920;
    config.height    = 1200;
    config.resizable = false;

    auto window_result = create_window(config);
    ASSERT_TRUE(window_result.has_value());

    auto device_result = WgpuDevice::create(window_result.value().get(), false);
    ASSERT_TRUE(device_result.has_value());

    auto& device = device_result.value();
    auto  vp     = device.viewport();

    // Should letterbox (black bars top/bottom)
    EXPECT_EQ(vp.width, 1920u);
    EXPECT_EQ(vp.height, 1080u);
    EXPECT_EQ(vp.x, 0u);
    EXPECT_EQ(vp.y, 60u); // (1200 - 1080) / 2
}

TEST_F(WgpuDeviceTest, Viewport_Auto_UsesFullWindow)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();
    device.set_aspect_ratio(AspectRatio::Auto);

    auto vp = device.viewport();

    // Auto mode uses full window
    EXPECT_EQ(vp.x, 0u);
    EXPECT_EQ(vp.y, 0u);
    EXPECT_EQ(vp.width, 800u);
    EXPECT_EQ(vp.height, 600u);
}

TEST_F(WgpuDeviceTest, ResizeWithAspectRatio_UpdatesViewport)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Resize window
    auto resize_result = device.resize(1600, 900);
    ASSERT_TRUE(resize_result.has_value());

    auto vp = device.viewport();

    // Should calculate viewport for 16:9 aspect ratio
    // 1600x900 is exactly 16:9, so should use full window
    EXPECT_EQ(vp.width, 1600u);
    EXPECT_EQ(vp.height, 900u);
}

TEST_F(WgpuDeviceTest, CustomAspectRatio_UsesCustomValue)
{
    auto result = WgpuDevice::create(_window.get(), false);
    ASSERT_TRUE(result.has_value());

    auto& device = result.value();

    // Set custom cinemascope ratio (2.35:1)
    device.set_aspect_ratio(AspectRatio::Custom, 2.35f);

    auto vp = device.viewport();

    // With 800x600 window and 2.35:1 ratio, should letterbox heavily
    EXPECT_EQ(vp.width, 800u);
    EXPECT_LT(vp.height, 600u);
    EXPECT_GT(vp.y, 0u); // Should have top/bottom bars
}
