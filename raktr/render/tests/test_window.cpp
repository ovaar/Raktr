/*!
 * @file test_window.cpp
 * @brief Unit tests for Window abstraction.
 */

#include <gtest/gtest.h>
#include "window/window.h"

using namespace raktr::render;

/*!
 * @brief Test fixture for Window tests.
 */
class WindowTest : public ::testing::Test
{
protected:
    WindowConfig default_config()
    {
        WindowConfig config;
        config.width = 800;
        config.height = 600;
        config.title = "Test Window";
        config.resizable = false;
        config.fullscreen = false;
        return config;
    }
};

TEST_F(WindowTest, CreateWindow_WithValidConfig_Succeeds)
{
    // Arrange
    auto config = default_config();

    // Act
    auto result = create_window(config);

    // Assert
    ASSERT_TRUE(result.has_value()) << "Window creation should succeed";
    EXPECT_NE(result->get(), nullptr);
}

TEST_F(WindowTest, CreateWindow_ReturnsCorrectDimensions)
{
    // Arrange
    auto config = default_config();
    config.width = 1024;
    config.height = 768;

    // Act
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    // Assert
    EXPECT_EQ(window->width(), 1024);
    EXPECT_EQ(window->height(), 768);
}

TEST_F(WindowTest, ShouldClose_InitiallyFalse)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    // Act & Assert
    EXPECT_FALSE(window->should_close()) << "Newly created window should not want to close";
}

TEST_F(WindowTest, NativeHandle_NotNull)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    // Act
    void* handle = window->native_handle();

    // Assert
    EXPECT_NE(handle, nullptr) << "Native window handle should be valid";
}

TEST_F(WindowTest, PollEvents_DoesNotCrash)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    // Act & Assert - Should not crash
    EXPECT_NO_THROW(window->poll_events());
}

TEST_F(WindowTest, SwapBuffers_DoesNotCrash)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    // Act & Assert - Should not crash
    EXPECT_NO_THROW(window->swap_buffers());
}

TEST_F(WindowTest, MultipleWindows_CanCoexist)
{
    // Arrange
    auto config1 = default_config();
    config1.title = "Window 1";
    auto config2 = default_config();
    config2.title = "Window 2";

    // Act
    auto window1 = create_window(config1);
    auto window2 = create_window(config2);

    // Assert
    ASSERT_TRUE(window1.has_value());
    ASSERT_TRUE(window2.has_value());
    EXPECT_NE(window1->get(), window2->get());
    EXPECT_NE(window1->get()->native_handle(), window2->get()->native_handle());
}

TEST_F(WindowTest, SetResizeCallback_WithNullCallback_DoesNotCrash)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    // Act & Assert - Should not crash
    EXPECT_NO_THROW(window->set_resize_callback(nullptr));
}

TEST_F(WindowTest, SetResizeCallback_CanSetAndClearCallback)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    bool callback_called = false;
    auto callback = [&callback_called](uint32_t, uint32_t) {
        callback_called = true;
    };

    // Act - Set callback
    EXPECT_NO_THROW(window->set_resize_callback(callback));
    
    // Act - Clear callback
    EXPECT_NO_THROW(window->set_resize_callback(nullptr));
    
    // Callback was set and cleared without crashing
    SUCCEED();
}
