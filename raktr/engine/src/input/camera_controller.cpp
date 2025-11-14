/*!
 * @file camera_controller.cpp
 * @brief Implementation of CameraController.
 */

#include "input/camera_controller.h"
#include <cmath>
#include <glm/glm.hpp>

namespace raktr::engine::input
{

    void CameraController::bind_key(KeyCode key, CameraAction action)
    {
        _key_bindings[key] = std::move(action);
    }

    void CameraController::unbind_key(KeyCode key)
    {
        _key_bindings.erase(key);
    }

    void CameraController::clear_bindings()
    {
        _key_bindings.clear();
    }

    void CameraController::enable_mouse_look(bool enabled)
    {
        _mouse_look_enabled = enabled;
        if (!enabled)
        {
            _first_mouse = true; // Reset on re-enable
        }
    }

    void CameraController::set_mouse_sensitivity(float sensitivity)
    {
        _mouse_sensitivity = sensitivity;
    }

    void CameraController::update(const InputState& input, scene::Camera& camera, float delta_time)
    {
        // Process key bindings
        for (const auto& [key, action] : _key_bindings)
        {
            if (input.keys[key])
            {
                action(camera, delta_time);
            }
        }

        // Process mouse look
        if (_mouse_look_enabled)
        {
            process_mouse_look(input, camera);
        }
    }

    void CameraController::process_mouse_look(const InputState& input, scene::Camera& camera)
    {
        if (_first_mouse)
        {
            _last_mouse_x = input.mouse_x;
            _last_mouse_y = input.mouse_y;
            _first_mouse  = false;
            return;
        }

        double xoffset = input.mouse_x - _last_mouse_x;
        double yoffset = _last_mouse_y - input.mouse_y; // Reversed: y-coordinates go from bottom to top

        _last_mouse_x = input.mouse_x;
        _last_mouse_y = input.mouse_y;

        // Only update if mouse actually moved
        if (std::abs(xoffset) < 0.001 && std::abs(yoffset) < 0.001)
        {
            return;
        }

        xoffset *= static_cast<double>(_mouse_sensitivity);
        yoffset *= static_cast<double>(_mouse_sensitivity);

        float new_yaw   = camera.yaw() + static_cast<float>(xoffset);
        float new_pitch = camera.pitch() + static_cast<float>(yoffset);

        camera.set_rotation(new_yaw, new_pitch);
    }

    CameraController CameraController::fps_controller(float speed, float sensitivity)
    {
        CameraController controller;
        controller.set_mouse_sensitivity(sensitivity);

        // Forward/backward
        controller.bind_key(KeyCode::W, [speed](scene::Camera& cam, float dt)
                            {
                                glm::vec3 pos = cam.position();
                                cam.set_position(pos + cam.forward() * speed * dt);
                            });

        controller.bind_key(KeyCode::S, [speed](scene::Camera& cam, float dt)
                            {
                                glm::vec3 pos = cam.position();
                                cam.set_position(pos - cam.forward() * speed * dt);
                            });

        // Strafe left/right
        controller.bind_key(KeyCode::A, [speed](scene::Camera& cam, float dt)
                            {
                                glm::vec3 pos = cam.position();
                                cam.set_position(pos - cam.right() * speed * dt);
                            });

        controller.bind_key(KeyCode::D, [speed](scene::Camera& cam, float dt)
                            {
                                glm::vec3 pos = cam.position();
                                cam.set_position(pos + cam.right() * speed * dt);
                            });

        // Up/down
        controller.bind_key(KeyCode::Q, [speed](scene::Camera& cam, float dt)
                            {
                                glm::vec3 pos = cam.position();
                                cam.set_position(pos + cam.up() * speed * dt);
                            });

        controller.bind_key(KeyCode::E, [speed](scene::Camera& cam, float dt)
                            {
                                glm::vec3 pos = cam.position();
                                cam.set_position(pos - cam.up() * speed * dt);
                            });

        return controller;
    }

    CameraController CameraController::flight_controller(float speed, float sensitivity)
    {
        // Same as FPS for now - could add roll controls or unrestricted pitch later
        return fps_controller(speed, sensitivity);
    }

} // namespace raktr::engine::input
