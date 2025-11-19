/*!
 * @file hi_z_occlusion_pass.cpp
 * @brief WebGPU Hi-Z occlusion culling pass implementation.
 */

#include "hi_z_occlusion_pass.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend::wgpu
{

    HiZOcclusionPass::HiZOcclusionPass(
        occlusion::HiZBuffer*               hi_z_buffer,
        const std::vector<occlusion::AABB>* aabbs,
        const glm::mat4&                    view_projection,
        std::vector<bool>*                  visibility_results)
        : _hi_z_buffer(hi_z_buffer), _aabbs(aabbs), _view_projection(view_projection), _visibility_results(visibility_results)
    {
        if (_hi_z_buffer == nullptr)
        {
            spdlog::warn("HiZOcclusionPass: Hi-Z buffer is null, culling will be disabled");
        }
    }

    void HiZOcclusionPass::execute([[maybe_unused]] WgpuPassContext& ctx)
    {
        // Early out if no Hi-Z buffer or no objects
        if (_hi_z_buffer == nullptr || _aabbs == nullptr || _aabbs->empty())
        {
            // Mark all objects as visible
            if (_visibility_results && _aabbs)
            {
                _visibility_results->assign(_aabbs->size(), true);
            }
            return;
        }

        // Use previous frame's depth pyramid for temporal occlusion
        // (available via ctx.prev_frame_hi_z_pyramid if needed for custom implementations)
        //
        // For the first frame or if no previous pyramid exists, all objects will be visible
        // since the Hi-Z buffer will have no depth data yet.

        // Test visibility using previous frame's pyramid
        auto result = _hi_z_buffer->test_visibility(*_aabbs, _view_projection);

        if (result.has_value())
        {
            *_visibility_results = std::move(result.value());

            // Log culling statistics
            const auto& stats         = _hi_z_buffer->stats();
            uint32_t    visible_count = static_cast<uint32_t>(
                std::count(_visibility_results->begin(), _visibility_results->end(), true));
            uint32_t culled_count = static_cast<uint32_t>(_visibility_results->size()) - visible_count;
            float    culling_rate = _visibility_results->empty()
                                        ? 0.0f
                                        : (100.0f * culled_count / _visibility_results->size());

            spdlog::info("Hi-Z Culling: {}/{} visible ({:.1f}% culled), visibility test: {:.3f}ms",
                         visible_count,
                         _visibility_results->size(),
                         culling_rate,
                         stats.visibility_test_ms);
        }
        else
        {
            // Error during visibility testing - mark all visible as fallback
            spdlog::warn("HiZOcclusionPass: Visibility test failed, marking all objects visible");
            if (_visibility_results && _aabbs)
            {
                _visibility_results->assign(_aabbs->size(), true);
            }
        }
    }

    void HiZOcclusionPass::on_viewport_resize([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
    {
        // Hi-Z buffer will be recreated by the application, nothing to do here
    }

    void HiZOcclusionPass::update(const std::vector<occlusion::AABB>* aabbs, const glm::mat4& view_projection)
    {
        _aabbs           = aabbs;
        _view_projection = view_projection;
    }

} // namespace raktr::render::backend::wgpu
