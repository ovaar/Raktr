/*!
 * @file input_event.h
 * @brief Unified input event variant type.
 */

#ifndef RAKTR_ENGINE_INPUT_INPUT_EVENT_H
#define RAKTR_ENGINE_INPUT_INPUT_EVENT_H

#include "key_event.h"
#include "mouse_event.h"
#include <variant>

namespace raktr::engine
{

    /*!
     * @brief Variant type holding any input event.
     *
     * Prefer composition over inheritance using std::variant for type-safe
     * event handling without vtable overhead.
     *
     * @example
     * void handle_event(const InputEvent& event) {
     *     std::visit([](const auto& e) {
     *         using T = std::decay_t<decltype(e)>;
     *         if constexpr (std::is_same_v<T, KeyEvent>) {
     *             // Handle keyboard event
     *         } else if constexpr (std::is_same_v<T, MouseButtonEvent>) {
     *             // Handle mouse button
     *         }
     *     }, event);
     * }
     */
    using InputEvent = std::variant<
        KeyEvent,
        MouseButtonEvent,
        MouseMoveEvent,
        MouseScrollEvent>;

} // namespace raktr::engine

#endif // RAKTR_ENGINE_INPUT_INPUT_EVENT_H
