/*!
 * @file wgpu_pass_context.h
 * @brief WebGPU-specific pass execution context.
 */

#ifndef RAKTR_RENDER_WGPU_PASS_CONTEXT_H
#define RAKTR_RENDER_WGPU_PASS_CONTEXT_H

#include <cstdint>
#include <webgpu/webgpu.h>

namespace raktr::render::backend::wgpu
{
    // Forward declaration
    class WgpuDevice;

    /*!
     * @brief WebGPU-specific pass execution context.
     *
     * Contains WebGPU command encoder, texture views, and frame state.
     * This struct is backend-specific and lives in src/, not public API.
     *
     * @note Passes receive this context and extract WebGPU resources.
     */
    struct WgpuPassContext
    {
        //! Current frame index (for temporal techniques)
        uint32_t frame_index = 0;

        //! Command encoder for recording GPU commands
        WGPUCommandEncoder command_encoder = nullptr;

        //! Current frame's color render target
        WGPUTextureView color_target = nullptr;

        //! Current frame's depth render target
        WGPUTextureView depth_target = nullptr;

        //! Previous frame's Hi-Z pyramid (for temporal occlusion culling)
        WGPUTextureView prev_frame_hi_z_pyramid = nullptr;

        //! Viewport dimensions
        uint32_t viewport_width  = 0;
        uint32_t viewport_height = 0;

        //! GPU device access (non-owning reference)
        WgpuDevice* device = nullptr;

        // Future extensions:
        // UniformBuffer& global_uniforms;
        // Scene& scene;
        // GpuProfiler* profiler;
    };

} // namespace raktr::render::backend::wgpu

#endif // RAKTR_RENDER_WGPU_PASS_CONTEXT_H
