/*!
 * @file camera.cpp
 * @brief Camera implementation.
 */

#include "scene/camera.h"
#include "math/transform_types.h" // From render module
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>


namespace raktr::engine::scene
{

    Camera::Camera(const glm::vec3& position,
                   float            fov_degrees,
                   float            aspect_ratio,
                   float            near_plane,
                   float            far_plane)
        : _position(position), _yaw(-90.0F) // Looking down -Z by default
          ,
          _pitch(0.0F), _fov_degrees(fov_degrees), _aspect_ratio(aspect_ratio), _near_plane(near_plane), _far_plane(far_plane)
    {
        update_camera_vectors();
    }

    void Camera::process_input(const InputState& input, float delta_time)
    {
        // Handle movement (WASD)
        glm::vec3 movement(0.0F);

        if (input.keys[KeyCode::W])
        {
            movement += _forward;
        }
        if (input.keys[KeyCode::S])
        {
            movement -= _forward;
        }
        if (input.keys[KeyCode::A])
        {
            movement -= _right;
        }
        if (input.keys[KeyCode::D])
        {
            movement += _right;
        }

        // Normalize and scale by speed and delta time
        if (glm::length(movement) > 0.0F)
        {
            _position += glm::normalize(movement) * _movement_speed * delta_time;
        }

        // Handle mouse look
        if (_first_mouse)
        {
            _last_mouse_x = input.mouse_x;
            _last_mouse_y = input.mouse_y;
            _first_mouse  = false;
        }

        double xoffset = input.mouse_x - _last_mouse_x;
        double yoffset = _last_mouse_y - input.mouse_y; // Reversed: y-coordinates go from bottom to top

        _last_mouse_x = input.mouse_x;
        _last_mouse_y = input.mouse_y;

        // Only update if mouse actually moved
        if (std::abs(xoffset) > 0.001 || std::abs(yoffset) > 0.001)
        {
            xoffset *= static_cast<double>(_mouse_sensitivity);
            yoffset *= static_cast<double>(_mouse_sensitivity);

            _yaw += static_cast<float>(xoffset);
            _pitch += static_cast<float>(yoffset);

            // Clamp pitch to prevent gimbal lock
            if (_pitch > 89.0F)
            {
                _pitch = 89.0F;
            }
            if (_pitch < -89.0F)
            {
                _pitch = -89.0F;
            }

            update_camera_vectors();
        }
    }

    void Camera::set_position(const glm::vec3& pos)
    {
        _position = pos;
    }

    void Camera::set_rotation(float yaw, float pitch)
    {
        _yaw   = yaw;
        _pitch = pitch;

        // Clamp pitch
        if (_pitch > 89.0F)
        {
            _pitch = 89.0F;
        }
        if (_pitch < -89.0F)
        {
            _pitch = -89.0F;
        }

        update_camera_vectors();
    }

    void Camera::set_movement_speed(float speed)
    {
        _movement_speed = speed;
    }

    void Camera::set_mouse_sensitivity(float sensitivity)
    {
        _mouse_sensitivity = sensitivity;
    }

    glm::vec3 Camera::position() const
    {
        return _position;
    }

    glm::vec3 Camera::forward() const
    {
        return _forward;
    }

    glm::vec3 Camera::right() const
    {
        return _right;
    }

    glm::vec3 Camera::up() const
    {
        return _up;
    }

    float Camera::yaw() const
    {
        return _yaw;
    }

    float Camera::pitch() const
    {
        return _pitch;
    }

    raktr::render::math::View Camera::view() const
    {
        return raktr::render::math::View::look_at(_position, _position + _forward, _up);
    }

    raktr::render::math::Perspective Camera::projection() const
    {
        return raktr::render::math::Perspective::from_fov_degrees(
            _fov_degrees,
            _aspect_ratio,
            _near_plane,
            _far_plane);
    }

    void Camera::set_aspect_ratio(float aspect)
    {
        _aspect_ratio = aspect;
    }

    void Camera::set_fov(float fov_degrees)
    {
        _fov_degrees = fov_degrees;
    }

    void Camera::update_camera_vectors()
    {
        // Calculate forward vector from yaw and pitch
        glm::vec3 front;
        front.x  = std::cos(glm::radians(_yaw)) * std::cos(glm::radians(_pitch));
        front.y  = std::sin(glm::radians(_pitch));
        front.z  = std::sin(glm::radians(_yaw)) * std::cos(glm::radians(_pitch));
        _forward = glm::normalize(front);

        // Calculate right and up vectors
        _right = glm::normalize(glm::cross(_forward, glm::vec3(0.0F, 1.0F, 0.0F))); // World up
        _up    = glm::normalize(glm::cross(_right, _forward));
    }

} // namespace raktr::engine::scene
