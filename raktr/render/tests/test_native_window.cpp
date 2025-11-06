/*!
 * @file test_native_window.cpp
 * @brief Tests for NativeWindow wrapping external window handles.
 */

#include "window/window.h"
#include <gtest/gtest.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace raktr::render;

class NativeWindowTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
#ifdef _WIN32
        // Create a dummy HWND for testing
        // Using GetDesktopWindow() as a valid window handle
        _test_handle = GetDesktopWindow();
#else
        // For non-Windows platforms, use a fake pointer
        _test_handle = reinterpret_cast<void*>(0x12345678);
#endif
    }

    void* _test_handle = nullptr;
};

TEST_F(NativeWindowTest, CreateFromNative_WithValidHandle_Succeeds)
{
    auto result = create_window_from_native(_test_handle, 1920, 1080);
    ASSERT_TRUE(result.has_value()) << "Failed to create window from native handle";
    
    auto& window = result.value();
    EXPECT_EQ(window->width(), 1920);
    EXPECT_EQ(window->height(), 1080);
    EXPECT_EQ(window->native_handle(), _test_handle);
}

TEST_F(NativeWindowTest, CreateFromNative_WithNullHandle_Fails)
{
    auto result = create_window_from_native(nullptr, 1920, 1080);
    EXPECT_FALSE(result.has_value()) << "Should fail with null handle";
}

TEST_F(NativeWindowTest, CreateFromNative_WithZeroWidth_Fails)
{
    auto result = create_window_from_native(_test_handle, 0, 1080);
    EXPECT_FALSE(result.has_value()) << "Should fail with zero width";
}

TEST_F(NativeWindowTest, CreateFromNative_WithZeroHeight_Fails)
{
    auto result = create_window_from_native(_test_handle, 1920, 0);
    EXPECT_FALSE(result.has_value()) << "Should fail with zero height";
}

TEST_F(NativeWindowTest, ShouldClose_InitiallyFalse)
{
    auto result = create_window_from_native(_test_handle, 800, 600);
    ASSERT_TRUE(result.has_value());
    
    auto& window = result.value();
    EXPECT_FALSE(window->should_close()) << "Window should not be marked for closing initially";
}

TEST_F(NativeWindowTest, PollEvents_DoesNotCrash)
{
    auto result = create_window_from_native(_test_handle, 800, 600);
    ASSERT_TRUE(result.has_value());
    
    auto& window = result.value();
    EXPECT_NO_THROW(window->poll_events()) << "poll_events() should not crash (is a no-op)";
}

TEST_F(NativeWindowTest, SwapBuffers_DoesNotCrash)
{
    auto result = create_window_from_native(_test_handle, 800, 600);
    ASSERT_TRUE(result.has_value());
    
    auto& window = result.value();
    EXPECT_NO_THROW(window->swap_buffers()) << "swap_buffers() should not crash (is a no-op)";
}

TEST_F(NativeWindowTest, NativeHandle_ReturnsSameHandle)
{
    auto result = create_window_from_native(_test_handle, 800, 600);
    ASSERT_TRUE(result.has_value());
    
    auto& window = result.value();
    EXPECT_EQ(window->native_handle(), _test_handle) 
        << "native_handle() should return the same handle passed during creation";
}

TEST_F(NativeWindowTest, Destruction_DoesNotDestroyHandle)
{
    // This test verifies that destroying the Window does NOT destroy the native handle
    // We can't directly test this without platform-specific verification, but we can
    // verify no crashes occur
    
    {
        auto result = create_window_from_native(_test_handle, 800, 600);
        ASSERT_TRUE(result.has_value());
        // Window goes out of scope here
    }
    
    // If the test reaches here without crashing, the handle wasn't destroyed
    SUCCEED() << "Window destruction did not crash (handle not destroyed)";
    
#ifdef _WIN32
    // On Windows, verify the handle is still valid
    EXPECT_TRUE(IsWindow(static_cast<HWND>(_test_handle))) 
        << "Window handle should still be valid after NativeWindow destruction";
#endif
}

TEST_F(NativeWindowTest, MultipleInstances_CanShareHandle)
{
    auto window1 = create_window_from_native(_test_handle, 800, 600);
    auto window2 = create_window_from_native(_test_handle, 1024, 768);
    
    ASSERT_TRUE(window1.has_value());
    ASSERT_TRUE(window2.has_value());
    
    EXPECT_EQ(window1.value()->native_handle(), window2.value()->native_handle())
        << "Multiple NativeWindow instances can wrap the same handle";
}

TEST_F(NativeWindowTest, SetResizeCallback_CanSetCallback)
{
    auto result = create_window_from_native(_test_handle, 800, 600);
    ASSERT_TRUE(result.has_value());
    auto& window = result.value();

    bool callback_called = false;
    window->set_resize_callback([&callback_called](uint32_t, uint32_t) {
        callback_called = true;
    });

    // Callback set successfully (test that it doesn't crash)
    EXPECT_FALSE(callback_called) << "Callback should not be called yet";
}

TEST_F(NativeWindowTest, IsFullscreen_InitiallyFalse)
{
    auto result = create_window_from_native(_test_handle, 800, 600);
    ASSERT_TRUE(result.has_value());
    auto& window = result.value();

    EXPECT_FALSE(window->is_fullscreen()) 
        << "NativeWindow should report not fullscreen initially";
}

TEST_F(NativeWindowTest, SetFullscreen_UpdatesInternalFlag)
{
    auto result = create_window_from_native(_test_handle, 800, 600);
    ASSERT_TRUE(result.has_value());
    auto& window = result.value();

    // Initially not fullscreen
    EXPECT_FALSE(window->is_fullscreen());

    // Set fullscreen flag
    window->set_fullscreen(true);
    EXPECT_TRUE(window->is_fullscreen()) 
        << "NativeWindow should report fullscreen after set_fullscreen(true)";

    // Clear fullscreen flag
    window->set_fullscreen(false);
    EXPECT_FALSE(window->is_fullscreen()) 
        << "NativeWindow should report windowed after set_fullscreen(false)";
}
