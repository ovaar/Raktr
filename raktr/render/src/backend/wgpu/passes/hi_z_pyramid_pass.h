/*!
 * @file hi_z_pyramid_pass.h
 * @brief WebGPU Hi-Z pyramid generation pass.
 */

#pragma once

#include "../wgpu_pass_context.h"
#include "occlusion/hi_z_buffer.h"
#include <string_view>
#include <webgpu/webgpu.h>


namespace raktr::render::backend::wgpu
{

    class HiZPyramidPass final
    {
    public:
        using ContextType = WgpuPassContext; // Required for type erasure

        HiZPyramidPass(
            occlusion::HiZBuffer* hi_z_buffer,
            WGPUTexture           depth_texture);

        ~HiZPyramidPass() = default;

        // Non-copyable, movable
        HiZPyramidPass(const HiZPyramidPass&)            = delete;
        HiZPyramidPass& operator=(const HiZPyramidPass&) = delete;
        HiZPyramidPass(HiZPyramidPass&&)                 = default;
        HiZPyramidPass& operator=(HiZPyramidPass&&)      = default;

        void                           execute(WgpuPassContext& ctx);
        void                           on_viewport_resize(uint32_t width, uint32_t height);
        [[nodiscard]] std::string_view name() const
        {
            return "HiZPyramidPass";
        }

        void update_depth_texture(WGPUTexture depth_texture);

    private:
        occlusion::HiZBuffer* _hi_z_buffer{ nullptr };
        WGPUTexture           _depth_texture{ nullptr };
    };

} // namespace raktr::render::backend::wgpu
