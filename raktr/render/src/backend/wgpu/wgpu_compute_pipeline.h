/*!
 * @file wgpu_compute_pipeline.h
 * @brief WebGPU compute pipeline implementation.
 */

#pragma once

#include "compute_pipeline.h"
#include <cstdint>
#include <webgpu/webgpu.h>

namespace raktr::render::backend::wgpu
{

    /*!
     * @brief WebGPU compute pipeline backend.
     *
     * Wraps WGPUComputePipeline created from descriptor.
     */
    class WgpuComputePipeline
    {
    public:
        /*!
         * @brief Create compute pipeline from descriptor.
         * @param device WebGPU device
         * @param descriptor Pipeline descriptor
         */
        WgpuComputePipeline(WGPUDevice device, const ComputePipelineDescriptor& descriptor);

        /*!
         * @brief Destructor - releases pipeline.
         */
        ~WgpuComputePipeline();

        // Non-copyable
        WgpuComputePipeline(const WgpuComputePipeline&)            = delete;
        WgpuComputePipeline& operator=(const WgpuComputePipeline&) = delete;

        // Movable
        WgpuComputePipeline(WgpuComputePipeline&& other) noexcept;
        WgpuComputePipeline& operator=(WgpuComputePipeline&& other) noexcept;

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
        [[nodiscard]] WGPUComputePipeline handle() const
        {
            return _pipeline;
        }

    private:
        WGPUComputePipeline _pipeline = nullptr;
    };

} // namespace raktr::render::backend::wgpu
