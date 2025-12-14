/*!
 * @file hi_z_buffer.h
 * @brief Hierarchical Z-Buffer for GPU-accelerated occlusion culling.
 *
 * Implements Hi-Z occlusion culling using depth pyramid generation via compute shader.
 * Provides efficient visibility testing for large numbers of objects (10,000+).
 */
#pragma once

#include "buffer.h"
#include <cstdint>
#include <expected>
#include <glm/glm.hpp>
#include <span>
#include <system_error>
#include <vector>

namespace raktr::render::occlusion
{

#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier

    /*!
     * @brief Axis-Aligned Bounding Box for visibility testing.
     * @note Uses 16-byte alignment to match GPU WGSL vec3 layout (32 bytes total per AABB).
     */
    struct alignas(16) AABB
    {
        alignas(16) glm::vec3 min; //!< Minimum corner in world space
        alignas(16) glm::vec3 max; //!< Maximum corner in world space

        /*!
         * @brief Compute AABB center.
         */
        [[nodiscard]] glm::vec3 center() const
        {
            return (min + max) * 0.5f;
        }

        /*!
         * @brief Compute AABB half-extents.
         */
        [[nodiscard]] glm::vec3 extents() const
        {
            return (max - min) * 0.5f;
        }
    };
#pragma warning(pop)

    /*!
     * @brief Hierarchical Z-Buffer for occlusion culling.
     *
     * Generates depth pyramid from depth buffer and tests object AABBs for visibility.
     * Two-stage pipeline: Frustum (coarse) → Hi-Z (fine) culling.
     *
     * @example
     * // Create Hi-Z buffer
     * auto hi_z = device->create_hi_z_buffer(1920, 1080);
     *
     * // After depth pre-pass:
     * hi_z->build_pyramid(depth_texture);
     *
     * // Test visibility
     * std::vector<AABB> aabbs = get_object_bounds();
     * auto visible = hi_z->test_visibility(aabbs, view_projection);
     * // visible[i] == true if aabbs[i] is visible
     */
    class HiZBuffer
    {
    public:
        virtual ~HiZBuffer() = default;

        /*!
         * @brief Build depth pyramid from depth buffer.
         *
         * Generates mipmap chain where each level stores maximum depth of 2×2 block from
         * previous level. Uses compute shader for GPU acceleration.
         *
         * @param depth_texture GPU depth buffer from current frame.
         * @return Success or error code.
         *
         * @note Should be called after depth pre-pass, before visibility testing.
         * @performance ~0.5-1.0ms for 1920×1080 depth buffer.
         */
        [[nodiscard]] virtual std::expected<void, std::error_code>
        build_pyramid(void* depth_texture) = 0;

        /*!
         * @brief Test AABBs for visibility against depth pyramid.
         *
         * Projects each AABB to screen space and samples appropriate mip level of depth pyramid.
         * Object is culled if AABB's nearest depth > depth pyramid value (conservative test).
         *
         * @param aabbs Object bounding boxes in world space.
         * @param view_projection Combined view * projection matrix.
         * @return Bitfield where bit i indicates visibility of aabbs[i].
         *
         * @note Results are conservative: may report occluded objects as visible, never vice versa.
         * @performance ~0.2-0.5ms for 10,000 AABBs on modern GPU.
         *
         * @example
         * auto visibility = hi_z->test_visibility(aabbs, camera.view_projection());
         * for (size_t i = 0; i < aabbs.size(); ++i) {
         *     if (visibility[i]) {
         *         render_object(i);
         *     }
         * }
         */
        [[nodiscard]] virtual std::expected<std::vector<bool>, std::error_code>
        test_visibility(std::span<const AABB> aabbs, const glm::mat4& view_projection) = 0;

        /*!
         * @brief Get number of mip levels in depth pyramid.
         */
        [[nodiscard]] virtual uint32_t mip_levels() const = 0;

        /*!
         * @brief Get depth pyramid texture width at level 0.
         */
        [[nodiscard]] virtual uint32_t width() const = 0;

        /*!
         * @brief Get depth pyramid texture height at level 0.
         */
        [[nodiscard]] virtual uint32_t height() const = 0;

        /*!
         * @brief Get last frame's performance metrics.
         */
        struct Stats
        {
            float    pyramid_build_ms;   //!< Time to build depth pyramid
            float    visibility_test_ms; //!< Time to test AABBs
            uint32_t objects_tested;     //!< Number of AABBs tested
            uint32_t objects_visible;    //!< Number passing visibility test
            uint32_t objects_culled;     //!< Number culled by Hi-Z
        };

        [[nodiscard]] virtual Stats stats() const = 0;
    };

} // namespace raktr::render::occlusion
