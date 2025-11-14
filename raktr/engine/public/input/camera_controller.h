/*!
 * @file camera_controller.h
 * @brief Controller for processing input and updating camera state.
 *
 * Separates input handling from camera logic using function composition.
 * Allows custom key bindings and different control schemes (FPS, flight, etc.).
 *
 * @example
 * CameraController controller;
 *
 * // Default FPS controls (WASD, QE, mouse look)
 * controller = CameraController::fps_controller(5.0f, 0.1f);
 *
 * // Or custom bindings
 * controller.bind_key(KeyCode::W, [speed](Camera& cam, float dt) {
 *     cam.set_position(cam.position() + cam.forward() * speed * dt);
 * });
 *
 * // In game loop
 * controller.update(input_state, camera, delta_time);
 */

#ifndef RAKTR_ENGINE_INPUT_CAMERA_CONTROLLER_H
#define RAKTR_ENGINE_INPUT_CAMERA_CONTROLLER_H

#include "input/input_event.h"
#include "scene/camera.h"
#include <functional>
#include <unordered_map>

namespace raktr::engine::input
{

    /*!
     * @brief Function type for camera control actions.
     *
     * Takes camera reference and delta time, performs camera update.
     */
    using CameraAction = std::function<void(scene::Camera&, float)>;

    /*!
     * @brief Controller for processing input and updating camera.
     *
     * Uses function composition to decouple input bindings from camera logic.
     * Supports custom key bindings and mouse look configuration.
     *
     * Thread Safety:
     * - Not thread-safe. Call update() from single thread only.
     */
    class CameraController
    {
    public:
        /*!
         * @brief Construct controller with no bindings.
         */
        CameraController() = default;

        /*!
         * @brief Bind a key to a camera action.
         *
         * @param key Key code to bind.
         * @param action Function to execute when key is pressed.
         *
         * @example
         * controller.bind_key(KeyCode::W, [speed = 5.0f](Camera& cam, float dt) {
         *     cam.set_position(cam.position() + cam.forward() * speed * dt);
         * });
         */
        void bind_key(KeyCode key, CameraAction action);

        /*!
         * @brief Remove key binding.
         */
        void unbind_key(KeyCode key);

        /*!
         * @brief Clear all key bindings.
         */
        void clear_bindings();

        /*!
         * @brief Enable or disable mouse look.
         *
         * @param enabled If true, mouse movement updates camera rotation.
         */
        void enable_mouse_look(bool enabled);

        /*!
         * @brief Set mouse sensitivity for look controls.
         *
         * @param sensitivity Rotation speed multiplier (typical: 0.05 - 0.5).
         */
        void set_mouse_sensitivity(float sensitivity);

        /*!
         * @brief Process input and update camera state.
         *
         * Executes all bound actions for pressed keys and handles mouse look.
         *
         * @param input Current input state from InputSystem.
         * @param camera Camera to update.
         * @param delta_time Time since last frame in seconds.
         */
        void update(const InputState& input, scene::Camera& camera, float delta_time);

        // ====================================================================
        // Preset Configurations
        // ====================================================================

        /*!
         * @brief Create FPS-style controller (WASD + QE + mouse look).
         *
         * Key bindings:
         * - W: Move forward
         * - S: Move backward
         * - A: Strafe left
         * - D: Strafe right
         * - Q: Move up
         * - E: Move down
         * - Mouse: Look (yaw/pitch)
         *
         * @param speed Movement speed in units per second.
         * @param sensitivity Mouse sensitivity for rotation.
         * @return Configured controller.
         */
        static CameraController fps_controller(float speed = 5.0f, float sensitivity = 0.1f);

        /*!
         * @brief Create flight-style controller (6DOF movement).
         *
         * Similar to FPS but with unrestricted pitch (no clamping).
         *
         * @param speed Movement speed in units per second.
         * @param sensitivity Mouse sensitivity for rotation.
         * @return Configured controller.
         */
        static CameraController flight_controller(float speed = 5.0f, float sensitivity = 0.1f);

    private:
        std::unordered_map<KeyCode, CameraAction> _key_bindings;

        bool  _mouse_look_enabled = true;
        float _mouse_sensitivity  = 0.1f;

        // Mouse tracking for delta calculation
        double _last_mouse_x = 0.0;
        double _last_mouse_y = 0.0;
        bool   _first_mouse  = true;

        void process_mouse_look(const InputState& input, scene::Camera& camera);
    };

} // namespace raktr::engine::input

#endif // RAKTR_ENGINE_INPUT_CAMERA_CONTROLLER_H
