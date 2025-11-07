/*!
 * @file input_system.h
 * @brief Input system for processing window events and managing input state.
 */

#ifndef RAKTR_ENGINE_INPUT_INPUT_SYSTEM_H
#define RAKTR_ENGINE_INPUT_INPUT_SYSTEM_H

#include "core/flag_set.h"
#include "input_event.h"
#include <memory>
#include <utility>

// Forward declaration to avoid including render headers
namespace raktr::render
{
    class Window;
}

namespace raktr::engine
{

    /*!
     * @brief Current input state snapshot.
     *
     * Contains the state of all keys and mouse buttons at a given moment.
     */
    struct InputState
    {
        FlagSet<KeyCode>     keys{};
        FlagSet<MouseButton> mouse_buttons{};
        double               mouse_x = 0.0;
        double               mouse_y = 0.0;
    };

    /*!
     * @brief Input system for processing user input events.
     *
     * Subscribes to Window callbacks, maps GLFW codes to engine codes,
     * maintains input state, and provides thread-safe event processing.
     *
     * Thread Safety:
     * - Callbacks are invoked on the main thread (GLFW constraint)
     * - Events are enqueued in a thread-safe queue
     * - process_events() can be called from worker threads
     *
     * @example
     * auto window = raktr::render::create_window(config);
     * InputSystem input_system(*window.value());
     *
     * // Main loop
     * while (!window->should_close()) {
     *     window->poll_events();  // Triggers callbacks (enqueues events)
     *     InputState input_state = input_system.process_events();
     *
     *     if (input_state.keys[KeyCode::Escape]) {
     *         break;
     *     }
     * }
     */
    class InputSystem
    {
    public:
        /*!
         * @brief Create input system and subscribe to window events.
         * @param window Window to receive input from. Must remain valid.
         */
        explicit InputSystem(raktr::render::Window& window);

        ~InputSystem();

        // Non-copyable, movable
        InputSystem(const InputSystem&)            = delete;
        InputSystem& operator=(const InputSystem&) = delete;
        InputSystem(InputSystem&&) noexcept;
        InputSystem& operator=(InputSystem&&) noexcept;

        /*!
         * @brief Map GLFW key code to engine KeyCode.
         * @param glfw_key GLFW_KEY_* constant.
         * @return Corresponding KeyCode or KeyCode::Unknown.
         */
        static KeyCode map_glfw_key(int glfw_key);

        /*!
         * @brief Map GLFW mouse button to engine MouseButton.
         * @param glfw_button GLFW_MOUSE_BUTTON_* constant.
         * @return Corresponding MouseButton.
         */
        static MouseButton map_glfw_mouse_button(int glfw_button);

        /*!
         * @brief Map GLFW modifier bitfield to KeyModifiers.
         * @param glfw_mods GLFW_MOD_* bitfield.
         * @return KeyModifiers struct.
         */
        static KeyModifiers map_glfw_mods(int glfw_mods);

        /*!
         * @brief Process all queued input events and return current state.
         *
         * Dequeues all events, updates internal state, and returns a snapshot.
         * Thread-safe - can be called from worker threads.
         *
         * @return Current input state snapshot.
         */
        InputState process_events();

        /*!
         * @brief Get current input state without processing events.
         * @return Current input state snapshot.
         */
        [[nodiscard]] InputState get_state() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _pimpl;
    };

} // namespace raktr::engine

#endif // RAKTR_ENGINE_INPUT_INPUT_SYSTEM_H
