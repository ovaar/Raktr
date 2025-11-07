/*!
 * @file test_input_system.cpp
 * @brief Unit tests for the InputSystem.
 */

#include "input/input_system.h"
#include "window/window.h"
#include <gtest/gtest.h>
#include <memory>

namespace raktr::engine::test
{

    // Mock Window for testing
    class MockWindow : public raktr::render::Window
    {
    public:
        MockWindow() = default;

        void poll_events() override
        {
        }
        void swap_buffers() override
        {
        }
        bool should_close() const override
        {
            return false;
        }
        uint32_t width() const override
        {
            return 800;
        }
        uint32_t height() const override
        {
            return 600;
        }
        void* native_handle() const override
        {
            return nullptr;
        }
        bool is_fullscreen() const override
        {
            return false;
        }
        void set_fullscreen(bool /* fullscreen */) override
        {
        }
        void set_resize_callback(raktr::render::ResizeCallback /* callback */) override
        {
        }

        // Store callbacks for testing
        void set_key_callback(raktr::render::KeyCallback callback) override
        {
            _key_callback = std::move(callback);
        }

        void set_mouse_button_callback(raktr::render::MouseButtonCallback callback) override
        {
            _mouse_button_callback = std::move(callback);
        }

        void set_cursor_pos_callback(raktr::render::CursorPosCallback callback) override
        {
            _cursor_pos_callback = std::move(callback);
        }

        void set_scroll_callback(raktr::render::ScrollCallback callback) override
        {
            _scroll_callback = std::move(callback);
        }

        // Trigger callbacks manually for testing
        void trigger_key(int key, int scancode, int action, int mods)
        {
            if (_key_callback)
            {
                _key_callback(key, scancode, action, mods);
            }
        }

        void trigger_mouse_button(int button, int action, int mods)
        {
            if (_mouse_button_callback)
            {
                _mouse_button_callback(button, action, mods);
            }
        }

        void trigger_cursor_pos(double xpos, double ypos)
        {
            if (_cursor_pos_callback)
            {
                _cursor_pos_callback(xpos, ypos);
            }
        }

        void trigger_scroll(double xoffset, double yoffset)
        {
            if (_scroll_callback)
            {
                _scroll_callback(xoffset, yoffset);
            }
        }

    private:
        raktr::render::KeyCallback         _key_callback;
        raktr::render::MouseButtonCallback _mouse_button_callback;
        raktr::render::CursorPosCallback   _cursor_pos_callback;
        raktr::render::ScrollCallback      _scroll_callback;
    };

    // Test GLFW key mapping
    TEST(InputSystem_map_glfw_key, maps_letter_keys_correctly)
    {
        EXPECT_EQ(InputSystem::map_glfw_key(65), KeyCode::A);
        EXPECT_EQ(InputSystem::map_glfw_key(90), KeyCode::Z);
    }

    TEST(InputSystem_map_glfw_key, maps_number_keys_correctly)
    {
        EXPECT_EQ(InputSystem::map_glfw_key(48), KeyCode::Num0);
        EXPECT_EQ(InputSystem::map_glfw_key(57), KeyCode::Num9);
    }

    TEST(InputSystem_map_glfw_key, maps_function_keys_correctly)
    {
        EXPECT_EQ(InputSystem::map_glfw_key(290), KeyCode::F1);
        EXPECT_EQ(InputSystem::map_glfw_key(301), KeyCode::F12);
    }

    TEST(InputSystem_map_glfw_key, maps_arrow_keys_correctly)
    {
        EXPECT_EQ(InputSystem::map_glfw_key(262), KeyCode::Right);
        EXPECT_EQ(InputSystem::map_glfw_key(263), KeyCode::Left);
        EXPECT_EQ(InputSystem::map_glfw_key(264), KeyCode::Down);
        EXPECT_EQ(InputSystem::map_glfw_key(265), KeyCode::Up);
    }

    TEST(InputSystem_map_glfw_key, maps_modifier_keys_correctly)
    {
        EXPECT_EQ(InputSystem::map_glfw_key(340), KeyCode::LeftShift);
        EXPECT_EQ(InputSystem::map_glfw_key(341), KeyCode::LeftControl);
        EXPECT_EQ(InputSystem::map_glfw_key(342), KeyCode::LeftAlt);
    }

    TEST(InputSystem_map_glfw_key, maps_unknown_key_to_unknown)
    {
        EXPECT_EQ(InputSystem::map_glfw_key(9999), KeyCode::Unknown);
        EXPECT_EQ(InputSystem::map_glfw_key(-1), KeyCode::Unknown);
    }

    // Test GLFW mouse button mapping
    TEST(InputSystem_map_glfw_mouse_button, maps_mouse_buttons_correctly)
    {
        EXPECT_EQ(InputSystem::map_glfw_mouse_button(0), MouseButton::Left);
        EXPECT_EQ(InputSystem::map_glfw_mouse_button(1), MouseButton::Right);
        EXPECT_EQ(InputSystem::map_glfw_mouse_button(2), MouseButton::Middle);
        EXPECT_EQ(InputSystem::map_glfw_mouse_button(3), MouseButton::Button4);
    }

    TEST(InputSystem_map_glfw_mouse_button, defaults_to_left_for_invalid_button)
    {
        EXPECT_EQ(InputSystem::map_glfw_mouse_button(9999), MouseButton::Left);
    }

    // Test GLFW modifier mapping
    TEST(InputSystem_map_glfw_mods, maps_shift_modifier)
    {
        auto mods = InputSystem::map_glfw_mods(0x0001); // GLFW_MOD_SHIFT
        EXPECT_TRUE(mods.shift);
        EXPECT_FALSE(mods.ctrl);
        EXPECT_FALSE(mods.alt);
        EXPECT_FALSE(mods.super);
    }

    TEST(InputSystem_map_glfw_mods, maps_control_modifier)
    {
        auto mods = InputSystem::map_glfw_mods(0x0002); // GLFW_MOD_CONTROL
        EXPECT_FALSE(mods.shift);
        EXPECT_TRUE(mods.ctrl);
        EXPECT_FALSE(mods.alt);
        EXPECT_FALSE(mods.super);
    }

    TEST(InputSystem_map_glfw_mods, maps_combined_modifiers)
    {
        auto mods = InputSystem::map_glfw_mods(0x0003); // SHIFT | CONTROL
        EXPECT_TRUE(mods.shift);
        EXPECT_TRUE(mods.ctrl);
        EXPECT_FALSE(mods.alt);
        EXPECT_FALSE(mods.super);
    }

    TEST(InputSystem_map_glfw_mods, maps_all_modifiers)
    {
        auto mods = InputSystem::map_glfw_mods(0x000F); // ALL
        EXPECT_TRUE(mods.shift);
        EXPECT_TRUE(mods.ctrl);
        EXPECT_TRUE(mods.alt);
        EXPECT_TRUE(mods.super);
    }

    // Test InputSystem event processing
    TEST(InputSystem_process_events, key_press_updates_state)
    {
        auto        window     = std::make_unique<MockWindow>();
        auto*       window_ptr = window.get();
        InputSystem input_system(*window);

        // Simulate key press (GLFW_KEY_A = 65, GLFW_PRESS = 1)
        window_ptr->trigger_key(65, 0, 1, 0);

        // Process events
        input_system.process_events();

        // Check state
        EXPECT_TRUE(input_system.is_key_pressed(KeyCode::A));
    }

    TEST(InputSystem_process_events, key_release_updates_state)
    {
        auto        window     = std::make_unique<MockWindow>();
        auto*       window_ptr = window.get();
        InputSystem input_system(*window);

        // Simulate key press then release
        window_ptr->trigger_key(65, 0, 1, 0); // Press
        input_system.process_events();
        EXPECT_TRUE(input_system.is_key_pressed(KeyCode::A));

        window_ptr->trigger_key(65, 0, 0, 0); // Release (GLFW_RELEASE = 0)
        input_system.process_events();
        EXPECT_FALSE(input_system.is_key_pressed(KeyCode::A));
    }

    TEST(InputSystem_process_events, multiple_keys_tracked_independently)
    {
        auto        window     = std::make_unique<MockWindow>();
        auto*       window_ptr = window.get();
        InputSystem input_system(*window);

        // Press A and B
        window_ptr->trigger_key(65, 0, 1, 0); // A
        window_ptr->trigger_key(66, 0, 1, 0); // B
        input_system.process_events();

        EXPECT_TRUE(input_system.is_key_pressed(KeyCode::A));
        EXPECT_TRUE(input_system.is_key_pressed(KeyCode::B));

        // Release A, keep B pressed
        window_ptr->trigger_key(65, 0, 0, 0); // Release A
        input_system.process_events();

        EXPECT_FALSE(input_system.is_key_pressed(KeyCode::A));
        EXPECT_TRUE(input_system.is_key_pressed(KeyCode::B));
    }

    TEST(InputSystem_process_events, mouse_button_press_updates_state)
    {
        auto        window     = std::make_unique<MockWindow>();
        auto*       window_ptr = window.get();
        InputSystem input_system(*window);

        // Simulate left mouse button press (GLFW_MOUSE_BUTTON_1 = 0, GLFW_PRESS = 1)
        window_ptr->trigger_mouse_button(0, 1, 0);
        input_system.process_events();

        EXPECT_TRUE(input_system.is_mouse_button_pressed(MouseButton::Left));
    }

    TEST(InputSystem_process_events, mouse_button_release_updates_state)
    {
        auto        window     = std::make_unique<MockWindow>();
        auto*       window_ptr = window.get();
        InputSystem input_system(*window);

        // Press and release
        window_ptr->trigger_mouse_button(0, 1, 0); // Press
        input_system.process_events();
        EXPECT_TRUE(input_system.is_mouse_button_pressed(MouseButton::Left));

        window_ptr->trigger_mouse_button(0, 0, 0); // Release
        input_system.process_events();
        EXPECT_FALSE(input_system.is_mouse_button_pressed(MouseButton::Left));
    }

    TEST(InputSystem_process_events, cursor_position_updates)
    {
        auto        window     = std::make_unique<MockWindow>();
        auto*       window_ptr = window.get();
        InputSystem input_system(*window);

        // Trigger cursor move
        window_ptr->trigger_cursor_pos(123.45, 678.90);
        input_system.process_events();

        auto [x, y] = input_system.get_mouse_position();
        EXPECT_DOUBLE_EQ(x, 123.45);
        EXPECT_DOUBLE_EQ(y, 678.90);
    }

    TEST(InputSystem_process_events, cursor_position_updates_multiple_times)
    {
        auto        window     = std::make_unique<MockWindow>();
        auto*       window_ptr = window.get();
        InputSystem input_system(*window);

        // First move
        window_ptr->trigger_cursor_pos(100.0, 200.0);
        input_system.process_events();
        auto [x1, y1] = input_system.get_mouse_position();
        EXPECT_DOUBLE_EQ(x1, 100.0);
        EXPECT_DOUBLE_EQ(y1, 200.0);

        // Second move
        window_ptr->trigger_cursor_pos(300.0, 400.0);
        input_system.process_events();
        auto [x2, y2] = input_system.get_mouse_position();
        EXPECT_DOUBLE_EQ(x2, 300.0);
        EXPECT_DOUBLE_EQ(y2, 400.0);
    }

    TEST(InputSystem_query, unpressed_key_returns_false)
    {
        auto        window = std::make_unique<MockWindow>();
        InputSystem input_system(*window);

        EXPECT_FALSE(input_system.is_key_pressed(KeyCode::A));
        EXPECT_FALSE(input_system.is_key_pressed(KeyCode::Escape));
    }

    TEST(InputSystem_query, unpressed_mouse_button_returns_false)
    {
        auto        window = std::make_unique<MockWindow>();
        InputSystem input_system(*window);

        EXPECT_FALSE(input_system.is_mouse_button_pressed(MouseButton::Left));
        EXPECT_FALSE(input_system.is_mouse_button_pressed(MouseButton::Right));
    }

    TEST(InputSystem_query, initial_mouse_position_is_zero)
    {
        auto        window = std::make_unique<MockWindow>();
        InputSystem input_system(*window);

        auto [x, y] = input_system.get_mouse_position();
        EXPECT_DOUBLE_EQ(x, 0.0);
        EXPECT_DOUBLE_EQ(y, 0.0);
    }

} // namespace raktr::engine::test
