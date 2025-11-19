/*!
 * @file hi_z_pyramid_pass.cpp
 * @brief WebGPU Hi-Z pyramid generation pass implementation.
 */

#include "hi_z_pyramid_pass.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend::wgpu
{

    HiZPyramidPass::HiZPyramidPass(
        occlusion::HiZBuffer* hi_z_buffer,
        WGPUTexture           depth_texture)
        : _hi_z_buffer(hi_z_buffer), _depth_texture(depth_texture)
    {
        if (_hi_z_buffer == nullptr)
        {
            spdlog::warn("HiZPyramidPass: Hi-Z buffer is null, pyramid generation will be skipped");
        }
    }

    void HiZPyramidPass::execute([[maybe_unused]] WgpuPassContext& ctx)
    {
        // Early out if no Hi-Z buffer or depth texture
        if (_hi_z_buffer == nullptr || _depth_texture == nullptr)
        {
            return;
        }

        // Build depth pyramid using compute shader
        // This pyramid will be used by next frame's occlusion culling pass
        auto result = _hi_z_buffer->build_pyramid(_depth_texture);

        if (result.has_value())
        {
            // Log pyramid build statistics
            const auto& stats = _hi_z_buffer->stats();
            spdlog::info("Hi-Z Pyramid: Built {}x{} pyramid with {} mip levels in {:.3f}ms",
                         _hi_z_buffer->width(),
                         _hi_z_buffer->height(),
                         _hi_z_buffer->mip_levels(),
                         stats.pyramid_build_ms);
        }
        else
        {
            spdlog::error("HiZPyramidPass: Failed to build depth pyramid: {}",
                          result.error().message());
        }
    }

    void HiZPyramidPass::on_viewport_resize([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
    {
        // Hi-Z buffer will be recreated by the application with new dimensions
        // Depth texture will be updated via update_depth_texture()
    }

    void HiZPyramidPass::update_depth_texture(WGPUTexture depth_texture)
    {
        _depth_texture = depth_texture;
    }

} // namespace raktr::render::backend::wgpu
