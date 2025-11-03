/*!
 * @file coordinate_system.h
 * @brief Right-hand coordinate system validation utilities.
 * 
 * Provides compile-time and runtime tools to enforce consistent use of
 * the right-hand coordinate system throughout Raktr.
 * 
 * @example
 * using namespace raktr::render::math;
 * 
 * // Validate basis is right-handed
 * bool valid = CoordinateBasis::is_right_handed(right, up, forward);
 * 
 * // Compute forward using right-hand rule
 * auto forward = CoordinateBasis::forward_from_right_up(
 *     glm::vec3(1, 0, 0),  // right
 *     glm::vec3(0, 1, 0)   // up
 * );
 * // => (0, 0, -1) for OpenGL/Vulkan convention
 */

#ifndef RAKTR_RENDER_MATH_COORDINATE_SYSTEM_H
#define RAKTR_RENDER_MATH_COORDINATE_SYSTEM_H

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace raktr::render::math
{
    /*!
     * @brief Marker type indicating right-hand coordinate system.
     * 
     * Right-hand rule convention:
     * - X-axis: Right (thumb)
     * - Y-axis: Up (index finger)
     * - Z-axis: Forward (middle finger, toward viewer = negative Z in OpenGL/Vulkan)
     * 
     * To verify: Point thumb right (+X), curl fingers up (+Y), middle finger points toward you (-Z).
     * 
     * Cross product: right × up = forward (using right-hand rule)
     */
    struct RightHanded {};

    /*!
     * @brief Utilities for validating and computing coordinate system bases.
     * 
     * Ensures consistent use of right-hand rule throughout the engine.
     */
    class CoordinateBasis
    {
    public:
        /*!
         * @brief Validates that three axes form a right-handed orthonormal basis.
         * 
         * @param right Right vector (typically +X)
         * @param up Up vector (typically +Y)  
         * @param forward Forward vector (typically -Z in OpenGL/Vulkan)
         * @return true if axes form right-handed basis, false otherwise
         * 
         * @note Axes should be normalized. Tolerates small numerical errors (< 0.01).
         * @note In OpenGL/Vulkan, forward = -(right × up) because forward is -Z.
         */
        [[nodiscard]] static bool is_right_handed(
            const glm::vec3& right,
            const glm::vec3& up,
            const glm::vec3& forward) noexcept
        {
            // Compute forward using right-hand rule: forward = -(right × up)
            // Negated because in OpenGL/Vulkan forward is -Z
            auto computed = -glm::cross(right, up);
            
            // Check if computed matches provided forward (within tolerance)
            return glm::dot(computed, forward) > 0.99f;
        }

        /*!
         * @brief Compute forward vector using right-hand rule.
         * 
         * Right-hand rule: forward = -(right × up)
         * In OpenGL/Vulkan, forward points toward viewer (-Z), but
         * right × up gives +Z, so we negate to get -Z.
         * 
         * @param right Right vector (typically +X)
         * @param up Up vector (typically +Y)
         * @return Forward vector following right-hand convention
         * 
         * @example
         * auto right = glm::vec3(1, 0, 0);
         * auto up = glm::vec3(0, 1, 0);
         * auto forward = CoordinateBasis::forward_from_right_up(right, up);
         * // => forward = (0, 0, -1) for OpenGL/Vulkan convention
         */
        [[nodiscard]] static glm::vec3 forward_from_right_up(
            const glm::vec3& right,
            const glm::vec3& up) noexcept
        {
            return -glm::normalize(glm::cross(right, up));
        }

        /*!
         * @brief Compute up vector using right-hand rule.
         * 
         * Right-hand rule: up = -(forward × right)
         * Negated to match OpenGL/Vulkan convention.
         * 
         * @param forward Forward vector
         * @param right Right vector
         * @return Up vector following right-hand convention
         */
        [[nodiscard]] static glm::vec3 up_from_forward_right(
            const glm::vec3& forward,
            const glm::vec3& right) noexcept
        {
            return -glm::normalize(glm::cross(forward, right));
        }

        /*!
         * @brief Compute right vector using right-hand rule.
         * 
         * Right-hand rule: right = -(up × forward)
         * Negated to match OpenGL/Vulkan convention.
         * 
         * @param up Up vector
         * @param forward Forward vector
         * @return Right vector following right-hand convention
         */
        [[nodiscard]] static glm::vec3 right_from_up_forward(
            const glm::vec3& up,
            const glm::vec3& forward) noexcept
        {
            return -glm::normalize(glm::cross(up, forward));
        }

        /*!
         * @brief Standard right-handed Y-up coordinate system.
         * 
         * - Right: +X (1, 0, 0)
         * - Up: +Y (0, 1, 0)
         * - Forward: -Z (0, 0, -1) [toward viewer in OpenGL/Vulkan]
         * 
         * This is the default convention for OpenGL, Vulkan, and WebGPU.
         */
        struct Standard
        {
            [[nodiscard]] static constexpr glm::vec3 right() noexcept { return glm::vec3(1, 0, 0); }
            [[nodiscard]] static constexpr glm::vec3 up() noexcept { return glm::vec3(0, 1, 0); }
            [[nodiscard]] static constexpr glm::vec3 forward() noexcept { return glm::vec3(0, 0, -1); }
        };
    };

} // namespace raktr::render::math

#endif // RAKTR_RENDER_MATH_COORDINATE_SYSTEM_H
