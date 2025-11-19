/*!
 * @file hi_z_occlusion_pass.h
 * @brief Render pass for GPU occlusion culling using previous frame's Hi-Z pyramid.
 */

#pragma once

#include "occlusion/hi_z_buffer.h"
#include "render_pass.h"
#include <glm/glm.hpp>
#include <span>
#include <vector>
#include <webgpu/webgpu.h>

namespace raktr::render
{

    /*!
     * @brief Performs GPU frustum + occlusion culling using temporal Hi-Z.
     *
     * Uses previous frame's depth pyramid for conservative occlusion testing.
     * Outputs visibility buffer (bitfield) for subsequent geometry pass.
     *
     * @note This pass does NOT render anything - it only computes visibility.
     *
     * @example
     * @code
     * auto culling_pass = std::make_unique<HiZOcclusionPass>(
     *     hi_z_buffer.get(),
     *     aabbs,
     *     view_projection
     * );
     * graph.add_pass(std::move(culling_pass));
     * @endcode
     */
    class HiZOcclusionPass final
    {
    public:
        using ContextType = PassContext; // Required for type erasure

        /*!
         * @brief Constructs occlusion culling pass.
         * @param hi_z_buffer Hi-Z buffer implementation (not owned).
         * @param aabbs Object bounding boxes in world space.
         * @param view_projection Camera view-projection matrix.
         * @param visibility_results Output buffer for visibility results (filled by execute()).
         */
        HiZOcclusionPass(
            occlusion::HiZBuffer*               hi_z_buffer,
            const std::vector<occlusion::AABB>* aabbs,
            const glm::mat4&                    view_projection,
            std::vector<bool>*                  visibility_results);

        ~HiZOcclusionPass() = default;

        // Non-copyable, movable
        HiZOcclusionPass(const HiZOcclusionPass&)            = delete;
        HiZOcclusionPass& operator=(const HiZOcclusionPass&) = delete;
        HiZOcclusionPass(HiZOcclusionPass&&)                 = default;
        HiZOcclusionPass& operator=(HiZOcclusionPass&&)      = default;

        // Required interface for type erasure
        void                           execute(PassContext& ctx);
        void                           on_viewport_resize(uint32_t width, uint32_t height);
        [[nodiscard]] std::string_view name() const
        {
            return "HiZOcclusionPass";
        }

        /*!
         * @brief Update scene data for next frame.
         * @param aabbs New object bounds.
         * @param view_projection New camera matrix.
         */
        void update(std::span<const occlusion::AABB> aabbs, const glm::mat4& view_projection);

    private:
        occlusion::HiZBuffer*               _hi_z_buffer{ nullptr };
        const std::vector<occlusion::AABB>* _aabbs{ nullptr };
        glm::mat4                           _view_projection;
        std::vector<bool>*                  _visibility_results{ nullptr };
    };

} // namespace raktr::render
