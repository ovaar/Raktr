/*!
 * @file wgpu_render_pipeline.h
 * @brief WebGPU render pipeline implementation.
 */

#pragma once

#include "render_pipeline.h"
#include <cstdint>
#include <webgpu/webgpu.h>

namespace raktr::render::backend::wgpu
{

    /*!
     * @brief WebGPU render pipeline backend.
     *
     * Wraps WGPURenderPipeline created from descriptor.
     */
    class WgpuRenderPipeline
    {
    public:
        /*!
         * @brief Create render pipeline from descriptor.
         * @param device WebGPU device
         * @param descriptor Pipeline descriptor
         */
        WgpuRenderPipeline(WGPUDevice device, const RenderPipelineDescriptor& descriptor);

        /*!
         * @brief Destructor - releases pipeline.
         */
        ~WgpuRenderPipeline();

        // Non-copyable
        WgpuRenderPipeline(const WgpuRenderPipeline&)            = delete;
        WgpuRenderPipeline& operator=(const WgpuRenderPipeline&) = delete;

        // Movable
        WgpuRenderPipeline(WgpuRenderPipeline&& other) noexcept;
        WgpuRenderPipeline& operator=(WgpuRenderPipeline&& other) noexcept;

        /*!
         * @brief Get native WebGPU pipeline handle.
         */
        [[nodiscard]] void* native_handle() const
        {
            return _pipeline;
        }

        /*!
         * @brief Get WebGPU pipeline handle.
         */
        [[nodiscard]] WGPURenderPipeline handle() const
        {
            return _pipeline;
        }

    private:
        WGPURenderPipeline _pipeline = nullptr;
    };

} // namespace raktr::render::backend::wgpu
