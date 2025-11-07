/*!
 * @file input_system.cpp
 * @brief Input system implementation.
 */

#include "input/input_system.h"
#include "window/window.h"
#include <algorithm>

// GLFW key codes (from GLFW/glfw3.h)
#define GLFW_KEY_SPACE         32
#define GLFW_KEY_APOSTROPHE    39
#define GLFW_KEY_COMMA         44
#define GLFW_KEY_MINUS         45
#define GLFW_KEY_PERIOD        46
#define GLFW_KEY_SLASH         47
#define GLFW_KEY_0             48
#define GLFW_KEY_1             49
#define GLFW_KEY_2             50
#define GLFW_KEY_3             51
#define GLFW_KEY_4             52
#define GLFW_KEY_5             53
#define GLFW_KEY_6             54
#define GLFW_KEY_7             55
#define GLFW_KEY_8             56
#define GLFW_KEY_9             57
#define GLFW_KEY_SEMICOLON     59
#define GLFW_KEY_EQUAL         61
#define GLFW_KEY_A             65
#define GLFW_KEY_B             66
#define GLFW_KEY_C             67
#define GLFW_KEY_D             68
#define GLFW_KEY_E             69
#define GLFW_KEY_F             70
#define GLFW_KEY_G             71
#define GLFW_KEY_H             72
#define GLFW_KEY_I             73
#define GLFW_KEY_J             74
#define GLFW_KEY_K             75
#define GLFW_KEY_L             76
#define GLFW_KEY_M             77
#define GLFW_KEY_N             78
#define GLFW_KEY_O             79
#define GLFW_KEY_P             80
#define GLFW_KEY_Q             81
#define GLFW_KEY_R             82
#define GLFW_KEY_S             83
#define GLFW_KEY_T             84
#define GLFW_KEY_U             85
#define GLFW_KEY_V             86
#define GLFW_KEY_W             87
#define GLFW_KEY_X             88
#define GLFW_KEY_Y             89
#define GLFW_KEY_Z             90
#define GLFW_KEY_LEFT_BRACKET  91
#define GLFW_KEY_BACKSLASH     92
#define GLFW_KEY_RIGHT_BRACKET 93
#define GLFW_KEY_GRAVE_ACCENT  96
#define GLFW_KEY_ESCAPE        256
#define GLFW_KEY_ENTER         257
#define GLFW_KEY_TAB           258
#define GLFW_KEY_BACKSPACE     259
#define GLFW_KEY_INSERT        260
#define GLFW_KEY_DELETE        261
#define GLFW_KEY_RIGHT         262
#define GLFW_KEY_LEFT          263
#define GLFW_KEY_DOWN          264
#define GLFW_KEY_UP            265
#define GLFW_KEY_PAGE_UP       266
#define GLFW_KEY_PAGE_DOWN     267
#define GLFW_KEY_HOME          268
#define GLFW_KEY_END           269
#define GLFW_KEY_CAPS_LOCK     280
#define GLFW_KEY_SCROLL_LOCK   281
#define GLFW_KEY_NUM_LOCK      282
#define GLFW_KEY_PRINT_SCREEN  283
#define GLFW_KEY_PAUSE         284
#define GLFW_KEY_F1            290
#define GLFW_KEY_F2            291
#define GLFW_KEY_F3            292
#define GLFW_KEY_F4            293
#define GLFW_KEY_F5            294
#define GLFW_KEY_F6            295
#define GLFW_KEY_F7            296
#define GLFW_KEY_F8            297
#define GLFW_KEY_F9            298
#define GLFW_KEY_F10           299
#define GLFW_KEY_F11           300
#define GLFW_KEY_F12           301
#define GLFW_KEY_F13           302
#define GLFW_KEY_F14           303
#define GLFW_KEY_F15           304
#define GLFW_KEY_F16           305
#define GLFW_KEY_F17           306
#define GLFW_KEY_F18           307
#define GLFW_KEY_F19           308
#define GLFW_KEY_F20           309
#define GLFW_KEY_F21           310
#define GLFW_KEY_F22           311
#define GLFW_KEY_F23           312
#define GLFW_KEY_F24           313
#define GLFW_KEY_F25           314
#define GLFW_KEY_KP_0          320
#define GLFW_KEY_KP_1          321
#define GLFW_KEY_KP_2          322
#define GLFW_KEY_KP_3          323
#define GLFW_KEY_KP_4          324
#define GLFW_KEY_KP_5          325
#define GLFW_KEY_KP_6          326
#define GLFW_KEY_KP_7          327
#define GLFW_KEY_KP_8          328
#define GLFW_KEY_KP_9          329
#define GLFW_KEY_KP_DECIMAL    330
#define GLFW_KEY_KP_DIVIDE     331
#define GLFW_KEY_KP_MULTIPLY   332
#define GLFW_KEY_KP_SUBTRACT   333
#define GLFW_KEY_KP_ADD        334
#define GLFW_KEY_KP_ENTER      335
#define GLFW_KEY_KP_EQUAL      336
#define GLFW_KEY_LEFT_SHIFT    340
#define GLFW_KEY_LEFT_CONTROL  341
#define GLFW_KEY_LEFT_ALT      342
#define GLFW_KEY_LEFT_SUPER    343
#define GLFW_KEY_RIGHT_SHIFT   344
#define GLFW_KEY_RIGHT_CONTROL 345
#define GLFW_KEY_RIGHT_ALT     346
#define GLFW_KEY_RIGHT_SUPER   347
#define GLFW_KEY_MENU          348

// GLFW mouse button codes
#define GLFW_MOUSE_BUTTON_1 0
#define GLFW_MOUSE_BUTTON_2 1
#define GLFW_MOUSE_BUTTON_3 2
#define GLFW_MOUSE_BUTTON_4 3
#define GLFW_MOUSE_BUTTON_5 4
#define GLFW_MOUSE_BUTTON_6 5
#define GLFW_MOUSE_BUTTON_7 6
#define GLFW_MOUSE_BUTTON_8 7

// GLFW action codes
#define GLFW_RELEASE 0
#define GLFW_PRESS   1
#define GLFW_REPEAT  2

// GLFW modifier bitfield
#define GLFW_MOD_SHIFT   0x0001
#define GLFW_MOD_CONTROL 0x0002
#define GLFW_MOD_ALT     0x0004
#define GLFW_MOD_SUPER   0x0008

namespace raktr::engine
{

    InputSystem::InputSystem(raktr::render::Window& window)
    {
        // Subscribe to window input callbacks
        window.set_key_callback([this](int key, int scancode, int action, int mods)
                                {
                                    on_key(key, scancode, action, mods);
                                });

        window.set_mouse_button_callback([this](int button, int action, int mods)
                                         {
                                             on_mouse_button(button, action, mods);
                                         });

        window.set_cursor_pos_callback([this](double xpos, double ypos)
                                       {
                                           on_cursor_pos(xpos, ypos);
                                       });

        window.set_scroll_callback([this](double xoffset, double yoffset)
                                   {
                                       on_scroll(xoffset, yoffset);
                                   });
    }

    KeyCode InputSystem::map_glfw_key(int glfw_key)
    {
        switch (glfw_key)
        {
            case GLFW_KEY_SPACE:
                return KeyCode::Space;
            case GLFW_KEY_APOSTROPHE:
                return KeyCode::Apostrophe;
            case GLFW_KEY_COMMA:
                return KeyCode::Comma;
            case GLFW_KEY_MINUS:
                return KeyCode::Minus;
            case GLFW_KEY_PERIOD:
                return KeyCode::Period;
            case GLFW_KEY_SLASH:
                return KeyCode::Slash;
            case GLFW_KEY_0:
                return KeyCode::Num0;
            case GLFW_KEY_1:
                return KeyCode::Num1;
            case GLFW_KEY_2:
                return KeyCode::Num2;
            case GLFW_KEY_3:
                return KeyCode::Num3;
            case GLFW_KEY_4:
                return KeyCode::Num4;
            case GLFW_KEY_5:
                return KeyCode::Num5;
            case GLFW_KEY_6:
                return KeyCode::Num6;
            case GLFW_KEY_7:
                return KeyCode::Num7;
            case GLFW_KEY_8:
                return KeyCode::Num8;
            case GLFW_KEY_9:
                return KeyCode::Num9;
            case GLFW_KEY_SEMICOLON:
                return KeyCode::Semicolon;
            case GLFW_KEY_EQUAL:
                return KeyCode::Equal;
            case GLFW_KEY_A:
                return KeyCode::A;
            case GLFW_KEY_B:
                return KeyCode::B;
            case GLFW_KEY_C:
                return KeyCode::C;
            case GLFW_KEY_D:
                return KeyCode::D;
            case GLFW_KEY_E:
                return KeyCode::E;
            case GLFW_KEY_F:
                return KeyCode::F;
            case GLFW_KEY_G:
                return KeyCode::G;
            case GLFW_KEY_H:
                return KeyCode::H;
            case GLFW_KEY_I:
                return KeyCode::I;
            case GLFW_KEY_J:
                return KeyCode::J;
            case GLFW_KEY_K:
                return KeyCode::K;
            case GLFW_KEY_L:
                return KeyCode::L;
            case GLFW_KEY_M:
                return KeyCode::M;
            case GLFW_KEY_N:
                return KeyCode::N;
            case GLFW_KEY_O:
                return KeyCode::O;
            case GLFW_KEY_P:
                return KeyCode::P;
            case GLFW_KEY_Q:
                return KeyCode::Q;
            case GLFW_KEY_R:
                return KeyCode::R;
            case GLFW_KEY_S:
                return KeyCode::S;
            case GLFW_KEY_T:
                return KeyCode::T;
            case GLFW_KEY_U:
                return KeyCode::U;
            case GLFW_KEY_V:
                return KeyCode::V;
            case GLFW_KEY_W:
                return KeyCode::W;
            case GLFW_KEY_X:
                return KeyCode::X;
            case GLFW_KEY_Y:
                return KeyCode::Y;
            case GLFW_KEY_Z:
                return KeyCode::Z;
            case GLFW_KEY_LEFT_BRACKET:
                return KeyCode::LeftBracket;
            case GLFW_KEY_BACKSLASH:
                return KeyCode::Backslash;
            case GLFW_KEY_RIGHT_BRACKET:
                return KeyCode::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT:
                return KeyCode::GraveAccent;
            case GLFW_KEY_ESCAPE:
                return KeyCode::Escape;
            case GLFW_KEY_ENTER:
                return KeyCode::Enter;
            case GLFW_KEY_TAB:
                return KeyCode::Tab;
            case GLFW_KEY_BACKSPACE:
                return KeyCode::Backspace;
            case GLFW_KEY_INSERT:
                return KeyCode::Insert;
            case GLFW_KEY_DELETE:
                return KeyCode::Delete;
            case GLFW_KEY_RIGHT:
                return KeyCode::Right;
            case GLFW_KEY_LEFT:
                return KeyCode::Left;
            case GLFW_KEY_DOWN:
                return KeyCode::Down;
            case GLFW_KEY_UP:
                return KeyCode::Up;
            case GLFW_KEY_PAGE_UP:
                return KeyCode::PageUp;
            case GLFW_KEY_PAGE_DOWN:
                return KeyCode::PageDown;
            case GLFW_KEY_HOME:
                return KeyCode::Home;
            case GLFW_KEY_END:
                return KeyCode::End;
            case GLFW_KEY_CAPS_LOCK:
                return KeyCode::CapsLock;
            case GLFW_KEY_SCROLL_LOCK:
                return KeyCode::ScrollLock;
            case GLFW_KEY_NUM_LOCK:
                return KeyCode::NumLock;
            case GLFW_KEY_PRINT_SCREEN:
                return KeyCode::PrintScreen;
            case GLFW_KEY_PAUSE:
                return KeyCode::Pause;
            case GLFW_KEY_F1:
                return KeyCode::F1;
            case GLFW_KEY_F2:
                return KeyCode::F2;
            case GLFW_KEY_F3:
                return KeyCode::F3;
            case GLFW_KEY_F4:
                return KeyCode::F4;
            case GLFW_KEY_F5:
                return KeyCode::F5;
            case GLFW_KEY_F6:
                return KeyCode::F6;
            case GLFW_KEY_F7:
                return KeyCode::F7;
            case GLFW_KEY_F8:
                return KeyCode::F8;
            case GLFW_KEY_F9:
                return KeyCode::F9;
            case GLFW_KEY_F10:
                return KeyCode::F10;
            case GLFW_KEY_F11:
                return KeyCode::F11;
            case GLFW_KEY_F12:
                return KeyCode::F12;
            case GLFW_KEY_F13:
                return KeyCode::F13;
            case GLFW_KEY_F14:
                return KeyCode::F14;
            case GLFW_KEY_F15:
                return KeyCode::F15;
            case GLFW_KEY_F16:
                return KeyCode::F16;
            case GLFW_KEY_F17:
                return KeyCode::F17;
            case GLFW_KEY_F18:
                return KeyCode::F18;
            case GLFW_KEY_F19:
                return KeyCode::F19;
            case GLFW_KEY_F20:
                return KeyCode::F20;
            case GLFW_KEY_F21:
                return KeyCode::F21;
            case GLFW_KEY_F22:
                return KeyCode::F22;
            case GLFW_KEY_F23:
                return KeyCode::F23;
            case GLFW_KEY_F24:
                return KeyCode::F24;
            case GLFW_KEY_F25:
                return KeyCode::F25;
            case GLFW_KEY_KP_0:
                return KeyCode::Kp0;
            case GLFW_KEY_KP_1:
                return KeyCode::Kp1;
            case GLFW_KEY_KP_2:
                return KeyCode::Kp2;
            case GLFW_KEY_KP_3:
                return KeyCode::Kp3;
            case GLFW_KEY_KP_4:
                return KeyCode::Kp4;
            case GLFW_KEY_KP_5:
                return KeyCode::Kp5;
            case GLFW_KEY_KP_6:
                return KeyCode::Kp6;
            case GLFW_KEY_KP_7:
                return KeyCode::Kp7;
            case GLFW_KEY_KP_8:
                return KeyCode::Kp8;
            case GLFW_KEY_KP_9:
                return KeyCode::Kp9;
            case GLFW_KEY_KP_DECIMAL:
                return KeyCode::KpDecimal;
            case GLFW_KEY_KP_DIVIDE:
                return KeyCode::KpDivide;
            case GLFW_KEY_KP_MULTIPLY:
                return KeyCode::KpMultiply;
            case GLFW_KEY_KP_SUBTRACT:
                return KeyCode::KpSubtract;
            case GLFW_KEY_KP_ADD:
                return KeyCode::KpAdd;
            case GLFW_KEY_KP_ENTER:
                return KeyCode::KpEnter;
            case GLFW_KEY_KP_EQUAL:
                return KeyCode::KpEqual;
            case GLFW_KEY_LEFT_SHIFT:
                return KeyCode::LeftShift;
            case GLFW_KEY_LEFT_CONTROL:
                return KeyCode::LeftControl;
            case GLFW_KEY_LEFT_ALT:
                return KeyCode::LeftAlt;
            case GLFW_KEY_LEFT_SUPER:
                return KeyCode::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT:
                return KeyCode::RightShift;
            case GLFW_KEY_RIGHT_CONTROL:
                return KeyCode::RightControl;
            case GLFW_KEY_RIGHT_ALT:
                return KeyCode::RightAlt;
            case GLFW_KEY_RIGHT_SUPER:
                return KeyCode::RightSuper;
            case GLFW_KEY_MENU:
                return KeyCode::Menu;
            default:
                return KeyCode::Unknown;
        }
    }

    MouseButton InputSystem::map_glfw_mouse_button(int glfw_button)
    {
        switch (glfw_button)
        {
            case GLFW_MOUSE_BUTTON_1:
                return MouseButton::Left;
            case GLFW_MOUSE_BUTTON_2:
                return MouseButton::Right;
            case GLFW_MOUSE_BUTTON_3:
                return MouseButton::Middle;
            case GLFW_MOUSE_BUTTON_4:
                return MouseButton::Button4;
            case GLFW_MOUSE_BUTTON_5:
                return MouseButton::Button5;
            case GLFW_MOUSE_BUTTON_6:
                return MouseButton::Button6;
            case GLFW_MOUSE_BUTTON_7:
                return MouseButton::Button7;
            case GLFW_MOUSE_BUTTON_8:
                return MouseButton::Button8;
            default:
                return MouseButton::Left;
        }
    }

    KeyModifiers InputSystem::map_glfw_mods(int glfw_mods)
    {
        KeyModifiers mods;
        mods.shift = (glfw_mods & GLFW_MOD_SHIFT) != 0;
        mods.ctrl  = (glfw_mods & GLFW_MOD_CONTROL) != 0;
        mods.alt   = (glfw_mods & GLFW_MOD_ALT) != 0;
        mods.super = (glfw_mods & GLFW_MOD_SUPER) != 0;
        return mods;
    }

    void InputSystem::process_events()
    {
        // Swap event queue (minimize lock time)
        std::vector<InputEvent> events;
        {
            std::lock_guard<std::mutex> lock(_event_queue_mutex);
            events = std::move(_event_queue);
            _event_queue.clear();
        }

        // Process all events
        for (const auto& event : events)
        {
            std::visit([this](const auto& e)
                       {
                           handle_event(e);
                       },
                       event);
        }
    }

    bool InputSystem::is_key_pressed(KeyCode key) const
    {
        auto it = _key_states.find(key);
        return it != _key_states.end() && it->second;
    }

    bool InputSystem::is_mouse_button_pressed(MouseButton button) const
    {
        auto it = _mouse_button_states.find(button);
        return it != _mouse_button_states.end() && it->second;
    }

    std::pair<double, double> InputSystem::get_mouse_position() const
    {
        return { _mouse_x, _mouse_y };
    }

    void InputSystem::on_key(int key, int scancode, int action, int mods)
    {
        KeyEvent event{
            .key      = map_glfw_key(key),
            .scancode = scancode,
            .action   = static_cast<KeyAction>(action),
            .mods     = map_glfw_mods(mods)
        };

        std::lock_guard<std::mutex> lock(_event_queue_mutex);
        _event_queue.emplace_back(std::move(event));
    }

    void InputSystem::on_mouse_button(int button, int action, int mods)
    {
        MouseButtonEvent event{
            .button = map_glfw_mouse_button(button),
            .action = static_cast<MouseAction>(action),
            .mods   = map_glfw_mods(mods)
        };

        std::lock_guard<std::mutex> lock(_event_queue_mutex);
        _event_queue.emplace_back(std::move(event));
    }

    void InputSystem::on_cursor_pos(double xpos, double ypos)
    {
        MouseMoveEvent event{
            .x = xpos,
            .y = ypos
        };

        std::lock_guard<std::mutex> lock(_event_queue_mutex);
        _event_queue.emplace_back(std::move(event));
    }

    void InputSystem::on_scroll(double xoffset, double yoffset)
    {
        MouseScrollEvent event{
            .xoffset = xoffset,
            .yoffset = yoffset
        };

        std::lock_guard<std::mutex> lock(_event_queue_mutex);
        _event_queue.emplace_back(std::move(event));
    }

    void InputSystem::handle_event(const KeyEvent& event)
    {
        // Update key state
        if (event.action == KeyAction::Press || event.action == KeyAction::Repeat)
        {
            _key_states[event.key] = true;
        }
        else if (event.action == KeyAction::Release)
        {
            _key_states[event.key] = false;
        }
    }

    void InputSystem::handle_event(const MouseButtonEvent& event)
    {
        // Update mouse button state
        if (event.action == MouseAction::Press)
        {
            _mouse_button_states[event.button] = true;
        }
        else if (event.action == MouseAction::Release)
        {
            _mouse_button_states[event.button] = false;
        }
    }

    void InputSystem::handle_event(const MouseMoveEvent& event)
    {
        // Update mouse position
        _mouse_x = event.x;
        _mouse_y = event.y;
    }

    void InputSystem::handle_event(const MouseScrollEvent& /* event */)
    {
        // Scroll events don't maintain state (they're deltas)
        // Applications should handle scroll events directly if needed
    }

} // namespace raktr::engine
