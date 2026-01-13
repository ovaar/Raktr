/*!
 * @file pass_context.h
 * @brief Context object carrying frame state for render passes.
 */

#ifndef RAKTR_RENDER_PASS_CONTEXT_H
#define RAKTR_RENDER_PASS_CONTEXT_H

#include <cstdint>

// Forward declare WebGPU types to avoid exposing webgpu.h in public API
using WGPUCommandEncoder = struct WGPUCommandEncoderImpl*;
using WGPUTextureView    = struct WGPUTextureViewImpl*;

namespace raktr::render
{

    /*!
     * @brief Context object passed to render passes during execution.
     *
     * Contains all frame-specific state and resources needed by passes.
     * Uses composition to reduce parameter lists and enable easy extension.
     *
     * @note This is a data carrier object - passes should not modify
     *       the device or other shared resources through this context.
     */
    struct PassContext
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

        // Future extensions:
        // UniformBuffer& global_uniforms;
        // Scene& scene;
        // GpuProfiler* profiler;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_PASS_CONTEXT_H
