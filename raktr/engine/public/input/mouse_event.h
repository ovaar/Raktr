/*!
 * @file mouse_event.h
 * @brief Mouse event structures and button definitions.
 */

#ifndef RAKTR_ENGINE_INPUT_MOUSE_EVENT_H
#define RAKTR_ENGINE_INPUT_MOUSE_EVENT_H

#include "key_event.h" // For KeyModifiers
#include <cstdint>

namespace raktr::engine
{

    /*!
     * @brief Mouse button codes mapped from GLFW.
     */
    enum class MouseButton : uint8_t
    {
        Left    = 0,
        Right   = 1,
        Middle  = 2,
        Button4 = 3,
        Button5 = 4,
        Button6 = 5,
        Button7 = 6,
        Button8 = 7,

        // Sentinel value for FlagSet (must be last)
        _
    };

    /*!
     * @brief Mouse button action (press, release).
     */
    enum class MouseAction : uint8_t
    {
        Release = 0,
        Press   = 1
    };

    /*!
     * @brief Mouse button event data.
     */
    struct MouseButtonEvent
    {
        MouseButton  button;
        MouseAction  action;
        KeyModifiers mods;
    };

    /*!
     * @brief Mouse cursor movement event data.
     */
    struct MouseMoveEvent
    {
        double x;
        double y;
    };

    /*!
     * @brief Mouse scroll event data.
     */
    struct MouseScrollEvent
    {
        double xoffset;
        double yoffset;
    };

} // namespace raktr::engine

#endif // RAKTR_ENGINE_INPUT_MOUSE_EVENT_H
