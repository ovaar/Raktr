/*!
 * @file transform_types.h
 * @brief Strongly-typed transformation wrappers for safer 3D math.
 * 
 * Provides type-safe wrappers around GLM matrices to prevent accidental
 * mixing of transformation types and make code more self-documenting.
 * 
 * @example
 * using namespace raktr::render::math;
 * 
 * // Type-safe transformations
 * Rotation model(angle, Axis::Y());
 * View view = View::look_at({0, 0, 3}, {0, 0, 0});
 * Perspective proj = Perspective::from_fov(45.0f, aspect, 0.1f, 100.0f);
 * 
 * // Compose transformations (enforces correct order)
 * ModelViewProjection mvp = proj * view * model;
 * 
 * // Upload to GPU
 * device->update_uniform_buffer(buf, mvp.to_bytes());
 */

#ifndef RAKTR_RENDER_MATH_TRANSFORM_TYPES_H
#define RAKTR_RENDER_MATH_TRANSFORM_TYPES_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <span>

namespace raktr::render::math
{
    // Forward declarations
    class Transform;
    class Rotation;
    class Translation;
    class Scale;
    class View;
    class Perspective;
    class Orthographic;
    class Model;
    class ModelView;
    class ModelViewProjection;

    /*!
     * @brief Axis helper for creating normalized axis vectors.
     */
    class Axis
    {
        glm::vec3 _axis;

    public:
        explicit Axis(float x, float y, float z) : _axis(glm::normalize(glm::vec3(x, y, z))) {}
        explicit Axis(const glm::vec3& v) : _axis(glm::normalize(v)) {}

        [[nodiscard]] const glm::vec3& vector() const noexcept { return _axis; }
        [[nodiscard]] static Axis X() { return Axis(1.0f, 0.0f, 0.0f); }
        [[nodiscard]] static Axis Y() { return Axis(0.0f, 1.0f, 0.0f); }
        [[nodiscard]] static Axis Z() { return Axis(0.0f, 0.0f, 1.0f); }
    };

    /*!
     * @brief Base transformation class using CRTP pattern.
     * 
     * Provides common matrix operations for all transformation types.
     */
    template<class Derived>
    class TransformBase
    {
    protected:
        glm::mat4 _matrix;

        explicit TransformBase(const glm::mat4& m) : _matrix(m) {}

    public:
        TransformBase(const TransformBase&) = default;
        TransformBase& operator=(const TransformBase&) = default;
        TransformBase(TransformBase&&) noexcept = default;
        TransformBase& operator=(TransformBase&&) noexcept = default;
        ~TransformBase() = default;

        [[nodiscard]] const glm::mat4& matrix() const noexcept { return _matrix; }
        [[nodiscard]] operator const glm::mat4&() const noexcept { return _matrix; }

        /*!
         * @brief Convert matrix to byte span for uniform buffer upload.
         */
        [[nodiscard]] std::span<const std::byte> to_bytes() const
        {
            const float* data = glm::value_ptr(_matrix);
            return std::as_bytes(std::span<const float>(data, 16));
        }

        /*!
         * @brief Get pointer to matrix data (column-major).
         */
        [[nodiscard]] const float* data() const noexcept
        {
            return glm::value_ptr(_matrix);
        }
    };

    /*!
     * @brief Rotation transformation around an axis.
     */
    class Rotation : public TransformBase<Rotation>
    {
    public:
        /*!
         * @brief Create rotation from angle and axis.
         * @param angle_radians Rotation angle in radians.
         * @param axis Rotation axis (automatically normalized).
         */
        Rotation(float angle_radians, const Axis& axis)
            : TransformBase(glm::rotate(glm::mat4(1.0f), angle_radians, axis.vector()))
        {
        }

        /*!
         * @brief Create rotation from angle and raw axis vector.
         * @param angle_radians Rotation angle in radians.
         * @param axis Raw axis vector (will be normalized).
         */
        Rotation(float angle_radians, const glm::vec3& axis)
            : Rotation(angle_radians, Axis(axis))
        {
        }

        /*!
         * @brief Create identity rotation (no rotation).
         */
        static Rotation identity()
        {
            return Rotation(0.0f, Axis::Y());
        }
    };

    /*!
     * @brief Translation transformation.
     */
    class Translation : public TransformBase<Translation>
    {
    public:
        /*!
         * @brief Create translation from vector.
         */
        explicit Translation(const glm::vec3& offset)
            : TransformBase(glm::translate(glm::mat4(1.0f), offset))
        {
        }

        /*!
         * @brief Create translation from coordinates.
         */
        Translation(float x, float y, float z)
            : Translation(glm::vec3(x, y, z))
        {
        }

        /*!
         * @brief Create identity translation (no movement).
         */
        static Translation identity()
        {
            return Translation(0.0f, 0.0f, 0.0f);
        }
    };

    /*!
     * @brief Scale transformation.
     */
    class Scale : public TransformBase<Scale>
    {
    public:
        /*!
         * @brief Create non-uniform scale.
         */
        explicit Scale(const glm::vec3& factors)
            : TransformBase(glm::scale(glm::mat4(1.0f), factors))
        {
        }

        /*!
         * @brief Create non-uniform scale from coordinates.
         */
        Scale(float x, float y, float z)
            : Scale(glm::vec3(x, y, z))
        {
        }

        /*!
         * @brief Create uniform scale.
         */
        static Scale uniform(float factor)
        {
            return Scale(factor, factor, factor);
        }

        /*!
         * @brief Create identity scale (no scaling).
         */
        static Scale identity()
        {
            return uniform(1.0f);
        }
    };

    /*!
     * @brief Model transformation (combination of rotation, translation, scale).
     */
    class Model : public TransformBase<Model>
    {
    public:
        /*!
         * @brief Create model transformation from matrix.
         */
        explicit Model(const glm::mat4& m) : TransformBase(m) {}

        /*!
         * @brief Create identity model transformation.
         */
        static Model identity()
        {
            return Model(glm::mat4(1.0f));
        }

        /*!
         * @brief Compose transformation with rotation.
         */
        Model operator*(const Rotation& r) const
        {
            return Model(_matrix * r.matrix());
        }

        /*!
         * @brief Compose transformation with translation.
         */
        Model operator*(const Translation& t) const
        {
            return Model(_matrix * t.matrix());
        }

        /*!
         * @brief Compose transformation with scale.
         */
        Model operator*(const Scale& s) const
        {
            return Model(_matrix * s.matrix());
        }
    };

    /*!
     * @brief View (camera) transformation.
     */
    class View : public TransformBase<View>
    {
    public:
        /*!
         * @brief Create view transformation from matrix.
         */
        explicit View(const glm::mat4& m) : TransformBase(m) {}

        /*!
         * @brief Create look-at view transformation.
         * @param eye Camera position.
         * @param target Point to look at.
         * @param up Up direction (default: Y-up).
         */
        static View look_at(const glm::vec3& eye, 
                           const glm::vec3& target,
                           const glm::vec3& up = {0.0f, 1.0f, 0.0f})
        {
            return View(glm::lookAt(eye, target, up));
        }

        /*!
         * @brief Create identity view (camera at origin, looking down -Z).
         */
        static View identity()
        {
            return View(glm::mat4(1.0f));
        }
    };

    /*!
     * @brief Perspective projection transformation.
     */
    class Perspective : public TransformBase<Perspective>
    {
    public:
        /*!
         * @brief Create perspective projection from matrix.
         */
        explicit Perspective(const glm::mat4& m) : TransformBase(m) {}

        /*!
         * @brief Create perspective projection from field of view.
         * @param fov_radians Vertical field of view in radians.
         * @param aspect Aspect ratio (width / height).
         * @param near_plane Near clipping plane distance.
         * @param far_plane Far clipping plane distance.
         */
        static Perspective from_fov(float fov_radians, float aspect,
                                    float near_plane, float far_plane)
        {
            return Perspective(glm::perspective(fov_radians, aspect, near_plane, far_plane));
        }

        /*!
         * @brief Create perspective projection from field of view in degrees.
         */
        static Perspective from_fov_degrees(float fov_degrees, float aspect,
                                           float near_plane, float far_plane)
        {
            return from_fov(glm::radians(fov_degrees), aspect, near_plane, far_plane);
        }
    };

    /*!
     * @brief Orthographic projection transformation.
     */
    class Orthographic : public TransformBase<Orthographic>
    {
    public:
        /*!
         * @brief Create orthographic projection from matrix.
         */
        explicit Orthographic(const glm::mat4& m) : TransformBase(m) {}

        /*!
         * @brief Create orthographic projection.
         * @param left Left coordinate of view volume.
         * @param right Right coordinate of view volume.
         * @param bottom Bottom coordinate of view volume.
         * @param top Top coordinate of view volume.
         * @param near_plane Near clipping plane distance.
         * @param far_plane Far clipping plane distance.
         */
        static Orthographic from_bounds(float left, float right,
                                       float bottom, float top,
                                       float near_plane, float far_plane)
        {
            return Orthographic(glm::ortho(left, right, bottom, top, near_plane, far_plane));
        }
    };

    /*!
     * @brief Combined model-view transformation.
     */
    class ModelView : public TransformBase<ModelView>
    {
    public:
        explicit ModelView(const glm::mat4& m) : TransformBase(m) {}
    };

    /*!
     * @brief Combined model-view-projection transformation.
     */
    class ModelViewProjection : public TransformBase<ModelViewProjection>
    {
    public:
        explicit ModelViewProjection(const glm::mat4& m) : TransformBase(m) {}
    };

    // Operator overloads enforcing correct transformation composition order

    // Rotation * Rotation => Model
    inline Model operator*(const Rotation& lhs, const Rotation& rhs)
    {
        return Model(lhs.matrix() * rhs.matrix());
    }

    // Rotation * Translation => Model
    inline Model operator*(const Rotation& lhs, const Translation& rhs)
    {
        return Model(lhs.matrix() * rhs.matrix());
    }

    // Translation * Rotation => Model
    inline Model operator*(const Translation& lhs, const Rotation& rhs)
    {
        return Model(lhs.matrix() * rhs.matrix());
    }

    // Scale * Rotation => Model
    inline Model operator*(const Scale& lhs, const Rotation& rhs)
    {
        return Model(lhs.matrix() * rhs.matrix());
    }

    // View * Model => ModelView
    inline ModelView operator*(const View& lhs, const Model& rhs)
    {
        return ModelView(lhs.matrix() * rhs.matrix());
    }

    // View * Rotation => ModelView (treat rotation as model)
    inline ModelView operator*(const View& lhs, const Rotation& rhs)
    {
        return ModelView(lhs.matrix() * rhs.matrix());
    }

    // Perspective * View => Projection-View (intermediate, rarely used directly)
    // Allow but return ModelViewProjection for consistency
    inline ModelViewProjection operator*(const Perspective& lhs, const View& rhs)
    {
        return ModelViewProjection(lhs.matrix() * rhs.matrix());
    }

    // Perspective * ModelView => ModelViewProjection
    inline ModelViewProjection operator*(const Perspective& lhs, const ModelView& rhs)
    {
        return ModelViewProjection(lhs.matrix() * rhs.matrix());
    }

    // Perspective * View * Model => MVP (most common pattern)
    // Helper to make: projection * view * model work naturally
    inline ModelViewProjection operator*(const ModelViewProjection& lhs, const Model& rhs)
    {
        return ModelViewProjection(lhs.matrix() * rhs.matrix());
    }

    inline ModelViewProjection operator*(const ModelViewProjection& lhs, const Rotation& rhs)
    {
        return ModelViewProjection(lhs.matrix() * rhs.matrix());
    }

    // Orthographic projections work the same way
    inline ModelViewProjection operator*(const Orthographic& lhs, const View& rhs)
    {
        return ModelViewProjection(lhs.matrix() * rhs.matrix());
    }

    inline ModelViewProjection operator*(const Orthographic& lhs, const ModelView& rhs)
    {
        return ModelViewProjection(lhs.matrix() * rhs.matrix());
    }

} // namespace raktr::render::math

#endif // RAKTR_RENDER_MATH_TRANSFORM_TYPES_H
