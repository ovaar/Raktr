/*!
 * @file test_wgpu_native_window.cpp
 * @brief Integration test for WgpuDevice with externally-provided window handles.
 */

#include "backend/wgpu/wgpu_device.h"
#include "window/window.h"
#include <gtest/gtest.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace raktr::render;
using namespace raktr::render::backend;

class WgpuNativeWindowTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
#ifdef _WIN32
        // Create a real hidden window for testing WebGPU surface creation
        WNDCLASSA wc = {};
        wc.lpfnWndProc = DefWindowProcA;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "RaktrTestWindow";
        RegisterClassA(&wc);

        _hwnd = CreateWindowExA(
            0, "RaktrTestWindow", "Test",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr
        );

        ASSERT_NE(_hwnd, nullptr) << "Failed to create test window";
#else
        // For non-Windows, use a fake handle (tests will be limited)
        _hwnd = reinterpret_cast<void*>(0x12345678);
#endif
    }

    void TearDown() override
    {
#ifdef _WIN32
        if (_hwnd)
        {
            DestroyWindow(static_cast<HWND>(_hwnd));
            _hwnd = nullptr;
        }
        UnregisterClassA("RaktrTestWindow", GetModuleHandle(nullptr));
#endif
    }

    void* _hwnd = nullptr;
};

TEST_F(WgpuNativeWindowTest, CreateDevice_WithNativeWindow_Succeeds)
{
    // Wrap the external HWND in a NativeWindow
    auto window_result = create_window_from_native(_hwnd, 800, 600);
    ASSERT_TRUE(window_result.has_value()) << "Failed to create NativeWindow";

    auto& window = window_result.value();

    // Create WgpuDevice with the native window
    auto device_result = WgpuDevice::create(window.get(), false);
    
#ifdef _WIN32
    // On Windows with real HWND, device creation should succeed
    ASSERT_TRUE(device_result.has_value()) << "Failed to create WgpuDevice with NativeWindow";

    auto& device = device_result.value();
    
    // Verify device is functional
    EXPECT_NE(device->wgpu_device(), nullptr);
    EXPECT_NE(device->wgpu_queue(), nullptr);
#else
    // On other platforms without proper setup, we just verify it doesn't crash
    // Device creation might fail without proper platform support
    SUCCEED() << "Test completed without crashing";
#endif
}

TEST_F(WgpuNativeWindowTest, RenderToNativeWindow_Succeeds)
{
#ifdef _WIN32
    // Wrap the external HWND
    auto window_result = create_window_from_native(_hwnd, 800, 600);
    ASSERT_TRUE(window_result.has_value());

    auto& window = window_result.value();

    // Create device
    auto device_result = WgpuDevice::create(window.get(), false);
    ASSERT_TRUE(device_result.has_value()) << "Failed to create WgpuDevice";

    auto& device = device_result.value();

    // Create a simple triangle
    float vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };
    auto vertex_data = std::as_bytes(std::span(vertices));
    auto vertex_buffer = device->create_vertex_buffer(vertex_data);
    ASSERT_TRUE(vertex_buffer.has_value());

    uint32_t indices[] = {0, 1, 2};
    auto index_data = std::as_bytes(std::span(indices));
    auto index_buffer = device->create_index_buffer(index_data);
    ASSERT_TRUE(index_buffer.has_value());

    // Render to the native window
    auto draw_result = device->draw_indexed(vertex_buffer.value(), index_buffer.value(), 3);
    EXPECT_TRUE(draw_result.has_value()) << "Failed to draw to native window";

    // Present
    EXPECT_NO_THROW(device->present());
#else
    SUCCEED() << "Test skipped on non-Windows platforms";
#endif
}

TEST_F(WgpuNativeWindowTest, MultipleFrames_WithNativeWindow_Succeeds)
{
#ifdef _WIN32
    auto window_result = create_window_from_native(_hwnd, 800, 600);
    ASSERT_TRUE(window_result.has_value());

    auto& window = window_result.value();
    auto device_result = WgpuDevice::create(window.get(), false);
    ASSERT_TRUE(device_result.has_value());

    auto& device = device_result.value();

    // Create geometry
    float vertices[] = {0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f};
    auto vertex_buffer = device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vertex_buffer.has_value());

    uint32_t indices[] = {0, 1, 2};
    auto index_buffer = device->create_index_buffer(std::as_bytes(std::span(indices)));
    ASSERT_TRUE(index_buffer.has_value());

    // Render multiple frames
    for (int i = 0; i < 3; ++i)
    {
        auto draw_result = device->draw_indexed(vertex_buffer.value(), index_buffer.value(), 3);
        EXPECT_TRUE(draw_result.has_value()) << "Failed on frame " << i;
        device->present();
    }
#else
    SUCCEED() << "Test skipped on non-Windows platforms";
#endif
}
