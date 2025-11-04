/*!
 * @file coordinate_spaces.h
 * @brief Strongly-typed coordinate space tagging for compile-time safety.
 * 
 * Prevents mixing vectors and transformations from different coordinate spaces
 * at compile time, catching common graphics programming errors early.
 * 
 * @example
 * using namespace raktr::render::math::spaces;
 * 
 * // Type-safe vectors in different spaces
 * LocalVector local_pos(1.0f, 2.0f, 3.0f);
 * WorldVector world_pos(5.0f, 10.0f, -3.0f);
 * 
 * // ❌ Won't compile - different spaces
 * // auto bad = local_pos + world_pos;
 * 
 * // ✅ Transform between spaces explicitly
 * SpaceTransform<LocalSpace, WorldSpace> model_transform(model_matrix);
 * WorldVector transformed = model_transform * local_pos;
 */

#ifndef RAKTR_RENDER_MATH_COORDINATE_SPACES_H
#define RAKTR_RENDER_MATH_COORDINATE_SPACES_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <type_traits>

namespace raktr::render::math::spaces
{
    // ========================================================================
    // Space Tag Types
    // ========================================================================

    /*!
     * @brief Local/Model space - object's own coordinate system.
     * Origin at object center, axes aligned with object.
     */
    struct LocalSpace {};

    /*!
     * @brief World space - global scene coordinate system.
     * Origin at scene center, axes aligned with world.
     */
    struct WorldSpace {};

    /*!
     * @brief View/Camera space - relative to camera.
     * Origin at camera position, +Z forward (looking direction).
     */
    struct ViewSpace {};

    /*!
     * @brief Clip space - after projection, before perspective divide.
     * Homogeneous coordinates, ready for GPU clipping.
     */
    struct ClipSpace {};

    /*!
     * @brief NDC (Normalized Device Coordinates) space.
     * After perspective divide, range [-1, 1] in all dimensions.
     */
    struct NDCSpace {};

    /*!
     * @brief Screen space - pixel coordinates.
     * Origin at top-left or bottom-left, units in pixels.
     */
    struct ScreenSpace {};

    /*!
     * @brief Tangent space - for normal mapping.
     * Per-vertex space with axes aligned to surface.
     */
    struct TangentSpace {};

    // ========================================================================
    // Strongly-Typed Vector
    // ========================================================================

    /*!
     * @brief Vector in a specific coordinate space.
     * 
     * Prevents mixing vectors from different spaces at compile time.
     * 
     * @tparam Space Coordinate space tag type (LocalSpace, WorldSpace, etc.)
     */
    template<typename Space>
    class Vector3
    {
        glm::vec3 _v;

    public:
        /*!
         * @brief Construct from components.
         */
        constexpr Vector3(float x, float y, float z) noexcept : _v(x, y, z) {}

        /*!
         * @brief Construct from GLM vector.
         */
        explicit constexpr Vector3(const glm::vec3& v) noexcept : _v(v) {}

        /*!
         * @brief Get underlying GLM vector (const).
         */
        [[nodiscard]] constexpr const glm::vec3& glm() const noexcept { return _v; }

        /*!
         * @brief Get underlying GLM vector (mutable).
         */
        [[nodiscard]] constexpr glm::vec3& glm() noexcept { return _v; }

        // Component access
        [[nodiscard]] constexpr float x() const noexcept { return _v.x; }
        [[nodiscard]] constexpr float y() const noexcept { return _v.y; }
        [[nodiscard]] constexpr float z() const noexcept { return _v.z; }

        [[nodiscard]] constexpr float& x() noexcept { return _v.x; }
        [[nodiscard]] constexpr float& y() noexcept { return _v.y; }
        [[nodiscard]] constexpr float& z() noexcept { return _v.z; }

        // Same-space operations
        [[nodiscard]] constexpr Vector3 operator+(const Vector3& other) const noexcept
        {
            return Vector3(_v + other._v);
        }

        [[nodiscard]] constexpr Vector3 operator-(const Vector3& other) const noexcept
        {
            return Vector3(_v - other._v);
        }

        [[nodiscard]] constexpr Vector3 operator*(float scalar) const noexcept
        {
            return Vector3(_v * scalar);
        }

        [[nodiscard]] constexpr Vector3 operator/(float scalar) const noexcept
        {
            return Vector3(_v / scalar);
        }

        [[nodiscard]] constexpr Vector3 operator-() const noexcept
        {
            return Vector3(-_v);
        }

        // In-place operations
        constexpr Vector3& operator+=(const Vector3& other) noexcept
        {
            _v += other._v;
            return *this;
        }

        constexpr Vector3& operator-=(const Vector3& other) noexcept
        {
            _v -= other._v;
            return *this;
        }

        constexpr Vector3& operator*=(float scalar) noexcept
        {
            _v *= scalar;
            return *this;
        }

        constexpr Vector3& operator/=(float scalar) noexcept
        {
            _v /= scalar;
            return *this;
        }

        /*!
         * @brief Dot product (same space only).
         */
        [[nodiscard]] constexpr float dot(const Vector3& other) const noexcept
        {
            return glm::dot(_v, other._v);
        }

        /*!
         * @brief Cross product (same space only).
         */
        [[nodiscard]] constexpr Vector3 cross(const Vector3& other) const noexcept
        {
            return Vector3(glm::cross(_v, other._v));
        }

        /*!
         * @brief Length (magnitude).
         */
        [[nodiscard]] float length() const noexcept
        {
            return glm::length(_v);
        }

        /*!
         * @brief Squared length (avoids sqrt).
         */
        [[nodiscard]] constexpr float length_squared() const noexcept
        {
            return glm::dot(_v, _v);
        }

        /*!
         * @brief Normalize vector.
         */
        [[nodiscard]] Vector3 normalized() const noexcept
        {
            return Vector3(glm::normalize(_v));
        }

        /*!
         * @brief Normalize in place.
         */
        Vector3& normalize() noexcept
        {
            _v = glm::normalize(_v);
            return *this;
        }
    };

    // Scalar * Vector3
    template<typename Space>
    [[nodiscard]] constexpr Vector3<Space> operator*(float scalar, const Vector3<Space>& v) noexcept
    {
        return v * scalar;
    }

    // ========================================================================
    // Space Transformation
    // ========================================================================

    /*!
     * @brief Transformation matrix between coordinate spaces.
     * 
     * Enforces correct space transitions at compile time.
     * 
     * @tparam FromSpace Source coordinate space
     * @tparam ToSpace Destination coordinate space
     */
    template<typename FromSpace, typename ToSpace>
    class SpaceTransform
    {
        glm::mat4 _m;

    public:
        /*!
         * @brief Construct from GLM matrix.
         */
        explicit constexpr SpaceTransform(const glm::mat4& m) noexcept : _m(m) {}

        /*!
         * @brief Get underlying matrix.
         */
        [[nodiscard]] constexpr const glm::mat4& matrix() const noexcept { return _m; }

        /*!
         * @brief Transform vector from FromSpace to ToSpace.
         */
        [[nodiscard]] Vector3<ToSpace> operator*(const Vector3<FromSpace>& v) const noexcept
        {
            glm::vec4 result = _m * glm::vec4(v.glm(), 1.0f);
            return Vector3<ToSpace>(glm::vec3(result) / result.w);
        }

        /*!
         * @brief Transform direction (w=0, no translation).
         */
        [[nodiscard]] Vector3<ToSpace> transform_direction(const Vector3<FromSpace>& v) const noexcept
        {
            glm::vec4 result = _m * glm::vec4(v.glm(), 0.0f);
            return Vector3<ToSpace>(glm::vec3(result));
        }
    };

    /*!
     * @brief Compose transformations (type-safe).
     * 
     * Resulting transform goes from A -> C via B.
     */
    template<typename A, typename B, typename C>
    [[nodiscard]] constexpr SpaceTransform<A, C> operator*(
        const SpaceTransform<B, C>& lhs,
        const SpaceTransform<A, B>& rhs) noexcept
    {
        return SpaceTransform<A, C>(lhs.matrix() * rhs.matrix());
    }

    // ========================================================================
    // Common Type Aliases
    // ========================================================================

    using LocalVector = Vector3<LocalSpace>;
    using WorldVector = Vector3<WorldSpace>;
    using ViewVector = Vector3<ViewSpace>;
    using ClipVector = Vector3<ClipSpace>;
    using NDCVector = Vector3<NDCSpace>;
    using ScreenVector = Vector3<ScreenSpace>;
    using TangentVector = Vector3<TangentSpace>;

    using LocalToWorld = SpaceTransform<LocalSpace, WorldSpace>;
    using WorldToView = SpaceTransform<WorldSpace, ViewSpace>;
    using ViewToClip = SpaceTransform<ViewSpace, ClipSpace>;
    using LocalToView = SpaceTransform<LocalSpace, ViewSpace>;
    using LocalToClip = SpaceTransform<LocalSpace, ClipSpace>;
    using WorldToClip = SpaceTransform<WorldSpace, ClipSpace>;

} // namespace raktr::render::math::spaces

#endif // RAKTR_RENDER_MATH_COORDINATE_SPACES_H
