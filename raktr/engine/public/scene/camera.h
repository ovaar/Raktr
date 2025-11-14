/*!
 * @file camera.h
 * @brief Camera class for 3D scene viewing with input control.
 *
 * Provides a first-person style camera with:
 * - Perspective projection (frustum)
 * - WASD movement controls
 * - Mouse look controls
 * - Integration with InputSystem
 *
 * @example
 * using namespace raktr::engine::scene;
 *
 * Camera camera(glm::vec3(0, 0, 3), 45.0f, 16.0f/9.0f);
 * camera.set_movement_speed(5.0f);
 * camera.set_mouse_sensitivity(0.1f);
 *
 * // In game loop:
 * InputState input = input_system.process_events();
 * camera.process_input(input, delta_time);
 *
 * View view = camera.view();
 * Perspective proj = camera.projection();
 * ModelViewProjection mvp = proj * view * model;
 */

#ifndef RAKTR_ENGINE_SCENE_CAMERA_H
#define RAKTR_ENGINE_SCENE_CAMERA_H

#include "input/input_event.h"
#include "input/input_system.h"
#include <glm/glm.hpp>

// Forward declare render math types to avoid circular dependency
namespace raktr::render::math
{
    class View;
    class Perspective;
} // namespace raktr::render::math

namespace raktr::engine::scene
{

    /*!
     * @brief First-person camera with perspective projection.
     *
     * Manages camera position, orientation, and projection parameters.
     * Processes WASD + mouse input for movement and look controls.
     *
     * Coordinate System:
     * - Right-handed (matches Raktr convention)
     * - Forward = -Z in view space
     * - Yaw = rotation around Y-axis (0° = looking +X, -90° = looking -Z)
     * - Pitch = rotation around X-axis (clamped to [-89°, 89°])
     *
     * Thread Safety:
     * - Not thread-safe. Call process_input() from single thread only.
     */
    class Camera
    {
    public:
        /*!
         * @brief Construct camera with position and projection parameters.
         *
         * @param position Initial camera position in world space.
         * @param fov_degrees Vertical field of view in degrees.
         * @param aspect_ratio Aspect ratio (width / height).
         * @param near_plane Near clipping plane distance (must be > 0).
         * @param far_plane Far clipping plane distance (must be > near_plane).
         *
         * @example
         * Camera cam(glm::vec3(0, 2, 5), 60.0f, 16.0f/9.0f);
         */
        Camera(const glm::vec3& position,
               float            fov_degrees,
               float            aspect_ratio,
               float            near_plane = 0.1f,
               float            far_plane  = 1000.0f);

        // ====================================================================
        // Camera Control
        // ====================================================================

        /*!
         * @brief Set camera position in world space.
         */
        void set_position(const glm::vec3& pos);

        /*!
         * @brief Set camera rotation using Euler angles.
         *
         * @param yaw Rotation around Y-axis in degrees.
         *            0° = looking +X, -90° = looking -Z (default)
         * @param pitch Rotation around X-axis in degrees.
         *              Clamped to [-89°, 89°] to prevent gimbal lock.
         */
        void set_rotation(float yaw, float pitch);

        // ====================================================================
        // Getters
        // ====================================================================

        /*!
         * @brief Get current camera position.
         */
        [[nodiscard]] glm::vec3 position() const;

        /*!
         * @brief Get forward direction vector (normalized).
         *
         * Points in the direction the camera is looking.
         * In default orientation: (0, 0, -1)
         */
        [[nodiscard]] glm::vec3 forward() const;

        /*!
         * @brief Get right direction vector (normalized).
         *
         * Points to the camera's right side.
         * In default orientation: (1, 0, 0)
         */
        [[nodiscard]] glm::vec3 right() const;

        /*!
         * @brief Get up direction vector (normalized).
         *
         * Points upward relative to camera orientation.
         * In default orientation: (0, 1, 0)
         */
        [[nodiscard]] glm::vec3 up() const;

        /*!
         * @brief Get yaw angle in degrees.
         */
        [[nodiscard]] float yaw() const;

        /*!
         * @brief Get pitch angle in degrees (clamped to [-89, 89]).
         */
        [[nodiscard]] float pitch() const;

        // ====================================================================
        // Matrix Generation
        // ====================================================================

        /*!
         * @brief Get view matrix for current camera state.
         *
         * Transforms from world space to view (camera) space.
         * Equivalent to glm::lookAt(position, position + forward, up).
         *
         * @return Type-safe View transformation.
         */
        [[nodiscard]] raktr::render::math::View view() const;

        /*!
         * @brief Get perspective projection matrix.
         *
         * Transforms from view space to clip space.
         *
         * @return Type-safe Perspective transformation.
         */
        [[nodiscard]] raktr::render::math::Perspective projection() const;

        // ====================================================================
        // Projection Updates
        // ====================================================================

        /*!
         * @brief Update aspect ratio (e.g., after window resize).
         *
         * @param aspect New aspect ratio (width / height).
         */
        void set_aspect_ratio(float aspect);

        /*!
         * @brief Update field of view.
         *
         * @param fov_degrees New vertical FOV in degrees.
         */
        void set_fov(float fov_degrees);

    private:
        // Camera state
        glm::vec3 _position;
        float     _yaw;   // Rotation around Y-axis (degrees)
        float     _pitch; // Rotation around X-axis (degrees, clamped)

        // Cached direction vectors (recomputed when rotation changes)
        glm::vec3 _forward;
        glm::vec3 _right;
        glm::vec3 _up;

        // Projection parameters
        float _fov_degrees;
        float _aspect_ratio;
        float _near_plane;
        float _far_plane;

        /*!
         * @brief Recompute forward/right/up vectors from yaw/pitch.
         */
        void update_camera_vectors();
    };

} // namespace raktr::engine::scene

#endif // RAKTR_ENGINE_SCENE_CAMERA_H
