/*!
 * @file test_window_input.cpp
 * @brief Unit tests for Window input callbacks.
 */

#include "window/window.h"
#include <gtest/gtest.h>

using namespace raktr::render;

/*!
 * @brief Test fixture for Window input callback tests.
 */
class WindowInputTest : public ::testing::Test
{
protected:
    WindowConfig default_config()
    {
        WindowConfig config;
        config.width      = 800;
        config.height     = 600;
        config.title      = "Input Test Window";
        config.resizable  = false;
        config.fullscreen = false;
        return config;
    }
};

TEST_F(WindowInputTest, SetKeyCallback_WithNullCallback_DoesNotCrash)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    // Act & Assert
    EXPECT_NO_THROW(window->set_key_callback(nullptr));
}

TEST_F(WindowInputTest, SetKeyCallback_CanSetAndClearCallback)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    bool callback_called = false;
    auto callback        = [&callback_called](int, int, int, int)
    {
        callback_called = true;
    };

    // Act - Set callback
    EXPECT_NO_THROW(window->set_key_callback(callback));

    // Act - Clear callback
    EXPECT_NO_THROW(window->set_key_callback(nullptr));

    // Callback was set and cleared without crashing
    SUCCEED();
}

TEST_F(WindowInputTest, SetMouseButtonCallback_WithNullCallback_DoesNotCrash)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    // Act & Assert
    EXPECT_NO_THROW(window->set_mouse_button_callback(nullptr));
}

TEST_F(WindowInputTest, SetMouseButtonCallback_CanSetAndClearCallback)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    bool callback_called = false;
    auto callback        = [&callback_called](int, int, int)
    {
        callback_called = true;
    };

    // Act - Set callback
    EXPECT_NO_THROW(window->set_mouse_button_callback(callback));

    // Act - Clear callback
    EXPECT_NO_THROW(window->set_mouse_button_callback(nullptr));

    // Callback was set and cleared without crashing
    SUCCEED();
}

TEST_F(WindowInputTest, SetCursorPosCallback_WithNullCallback_DoesNotCrash)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    // Act & Assert
    EXPECT_NO_THROW(window->set_cursor_pos_callback(nullptr));
}

TEST_F(WindowInputTest, SetCursorPosCallback_CanSetAndClearCallback)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    bool callback_called = false;
    auto callback        = [&callback_called](double, double)
    {
        callback_called = true;
    };

    // Act - Set callback
    EXPECT_NO_THROW(window->set_cursor_pos_callback(callback));

    // Act - Clear callback
    EXPECT_NO_THROW(window->set_cursor_pos_callback(nullptr));

    // Callback was set and cleared without crashing
    SUCCEED();
}

TEST_F(WindowInputTest, SetScrollCallback_WithNullCallback_DoesNotCrash)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    // Act & Assert
    EXPECT_NO_THROW(window->set_scroll_callback(nullptr));
}

TEST_F(WindowInputTest, SetScrollCallback_CanSetAndClearCallback)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    bool callback_called = false;
    auto callback        = [&callback_called](double, double)
    {
        callback_called = true;
    };

    // Act - Set callback
    EXPECT_NO_THROW(window->set_scroll_callback(callback));

    // Act - Clear callback
    EXPECT_NO_THROW(window->set_scroll_callback(nullptr));

    // Callback was set and cleared without crashing
    SUCCEED();
}

TEST_F(WindowInputTest, AllInputCallbacks_CanBeSetSimultaneously)
{
    // Arrange
    auto config = default_config();
    auto result = create_window(config);
    ASSERT_TRUE(result.has_value());
    auto& window = *result;

    int key_called          = 0;
    int mouse_button_called = 0;
    int cursor_pos_called   = 0;
    int scroll_called       = 0;

    // Act - Set all callbacks
    window->set_key_callback([&key_called](int, int, int, int)
                             {
                                 key_called++;
                             });

    window->set_mouse_button_callback([&mouse_button_called](int, int, int)
                                      {
                                          mouse_button_called++;
                                      });

    window->set_cursor_pos_callback([&cursor_pos_called](double, double)
                                    {
                                        cursor_pos_called++;
                                    });

    window->set_scroll_callback([&scroll_called](double, double)
                                {
                                    scroll_called++;
                                });

    // Assert - All callbacks set without crash
    SUCCEED();
}
