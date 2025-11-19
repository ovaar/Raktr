/*!
 * @file hi_z_occlusion_pass.h
 * @brief WebGPU occlusion culling pass using Hi-Z pyramid.
 */

#pragma once

#include "../wgpu_pass_context.h"
#include "occlusion/hi_z_buffer.h"
#include <glm/glm.hpp>
#include <string_view>
#include <vector>
#include <webgpu/webgpu.h>


namespace raktr::render::backend::wgpu
{

    class HiZOcclusionPass final
    {
    public:
        using ContextType = WgpuPassContext; // Required for type erasure

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

        void                           execute(WgpuPassContext& ctx);
        void                           on_viewport_resize(uint32_t width, uint32_t height);
        [[nodiscard]] std::string_view name() const
        {
            return "HiZOcclusionPass";
        }

        void update(const std::vector<occlusion::AABB>* aabbs, const glm::mat4& view_projection);

    private:
        occlusion::HiZBuffer*               _hi_z_buffer{ nullptr };
        const std::vector<occlusion::AABB>* _aabbs{ nullptr };
        glm::mat4                           _view_projection;
        std::vector<bool>*                  _visibility_results{ nullptr };
    };

} // namespace raktr::render::backend::wgpu
