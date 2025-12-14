/*!
 * @file wgpu_compute_pipeline.cpp
 * @brief WebGPU compute pipeline implementation.
 */

#include "wgpu_compute_pipeline.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend::wgpu
{

    WgpuComputePipeline::WgpuComputePipeline(WGPUDevice device, const ComputePipelineDescriptor& descriptor)
    {
        WGPUComputePipelineDescriptor pipeline_desc = {};
        pipeline_desc.label                         = descriptor.label.empty() ? WGPUStringView{ nullptr, 0 } : WGPUStringView{ descriptor.label.data(), descriptor.label.length() };
        pipeline_desc.compute.module                = static_cast<WGPUShaderModule>(descriptor.compute_shader.native_handle());
        pipeline_desc.compute.entryPoint            = WGPUStringView{ descriptor.compute_entry_point.data(), descriptor.compute_entry_point.length() };

        _pipeline = wgpuDeviceCreateComputePipeline(device, &pipeline_desc);

        if (!_pipeline)
        {
            spdlog::error("Failed to create compute pipeline: {}", descriptor.label);
        }
    }

    WgpuComputePipeline::~WgpuComputePipeline()
    {
        if (_pipeline)
        {
            wgpuComputePipelineRelease(_pipeline);
            _pipeline = nullptr;
        }
    }

    WgpuComputePipeline::WgpuComputePipeline(WgpuComputePipeline&& other) noexcept
        : _pipeline(other._pipeline)
    {
        other._pipeline = nullptr;
    }

    WgpuComputePipeline& WgpuComputePipeline::operator=(WgpuComputePipeline&& other) noexcept
    {
        if (this != &other)
        {
            if (_pipeline)
            {
                wgpuComputePipelineRelease(_pipeline);
            }
            _pipeline       = other._pipeline;
            other._pipeline = nullptr;
        }
        return *this;
    }

} // namespace raktr::render::backend::wgpu
