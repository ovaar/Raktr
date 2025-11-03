/*!
 * @file transform.h
 * @brief GLM-based transformation utilities for 3D rendering.
 * 
 * Provides convenient wrappers around GLM for common transformation operations.
 * All matrices are column-major, compatible with WGSL/GLSL shaders.
 */

#ifndef RAKTR_RENDER_MATH_TRANSFORM_H
#define RAKTR_RENDER_MATH_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <span>

namespace raktr::render::math
{
    /*!
     * @brief Create a perspective projection matrix.
     * 
     * @param fov_radians Field of view in radians.
     * @param aspect Aspect ratio (width / height).
     * @param near_plane Near clipping plane distance.
     * @param far_plane Far clipping plane distance.
     * @return 4x4 perspective projection matrix (column-major).
     * 
     * @example
     * auto proj = create_perspective(glm::radians(45.0f), 16.0f/9.0f, 0.1f, 100.0f);
     */
    inline glm::mat4 create_perspective(float fov_radians, float aspect, 
                                       float near_plane, float far_plane)
    {
        return glm::perspective(fov_radians, aspect, near_plane, far_plane);
    }

    /*!
     * @brief Create a view matrix for a camera looking at a target.
     * 
     * @param eye Camera position.
     * @param target Point the camera is looking at.
     * @param up Up direction vector (typically {0, 1, 0}).
     * @return 4x4 view matrix (column-major).
     * 
     * @example
     * auto view = create_look_at({0, 0, 3}, {0, 0, 0}, {0, 1, 0});
     */
    inline glm::mat4 create_look_at(const glm::vec3& eye, const glm::vec3& target, 
                                    const glm::vec3& up = {0.0f, 1.0f, 0.0f})
    {
        return glm::lookAt(eye, target, up);
    }

    /*!
     * @brief Create a rotation matrix from angle and axis.
     * 
     * @param angle_radians Rotation angle in radians.
     * @param axis Rotation axis (should be normalized).
     * @return 4x4 rotation matrix (column-major).
     * 
     * @example
     * auto rotation = create_rotation(glm::radians(45.0f), {0, 1, 0});
     */
    inline glm::mat4 create_rotation(float angle_radians, const glm::vec3& axis)
    {
        return glm::rotate(glm::mat4(1.0f), angle_radians, axis);
    }

    /*!
     * @brief Create a translation matrix.
     * 
     * @param translation Translation vector.
     * @return 4x4 translation matrix (column-major).
     * 
     * @example
     * auto translation = create_translation({1, 2, 3});
     */
    inline glm::mat4 create_translation(const glm::vec3& translation)
    {
        return glm::translate(glm::mat4(1.0f), translation);
    }

    /*!
     * @brief Create a scale matrix.
     * 
     * @param scale Scale factors for x, y, z axes.
     * @return 4x4 scale matrix (column-major).
     * 
     * @example
     * auto scale = create_scale({2, 2, 2});  // Uniform 2x scale
     */
    inline glm::mat4 create_scale(const glm::vec3& scale)
    {
        return glm::scale(glm::mat4(1.0f), scale);
    }

    /*!
     * @brief Convert GLM matrix to byte span for uniform buffer upload.
     * 
     * GLM matrices are already column-major, so this is a simple cast.
     * The returned span is valid as long as the matrix object exists.
     * 
     * @param matrix GLM matrix to convert.
     * @return Byte span suitable for update_uniform_buffer().
     * 
     * @example
     * glm::mat4 mvp = projection * view * model;
     * device->update_uniform_buffer(uniform_buf, matrix_to_bytes(mvp));
     */
    inline std::span<const std::byte> matrix_to_bytes(const glm::mat4& matrix)
    {
        const float* data = glm::value_ptr(matrix);
        return std::as_bytes(std::span<const float>(data, 16));
    }

} // namespace raktr::render::math

#endif // RAKTR_RENDER_MATH_TRANSFORM_H
