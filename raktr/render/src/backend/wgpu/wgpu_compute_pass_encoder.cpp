/*!
 * @file wgpu_compute_pass_encoder.cpp
 * @brief WebGPU compute pass encoder implementation.
 */

#include "wgpu_compute_pass_encoder.h"
#include <spdlog/spdlog.h>
namespace raktr::render::backend::wgpu
{
    void WgpuComputePassEncoder::set_pipeline(const render::ComputePipeline& pipeline) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuComputePassEncoder::set_pipeline: Invalid encoder");
            return;
        }

        WGPUComputePipeline wgpu_pipeline = static_cast<WGPUComputePipeline>(pipeline.native_handle());
        if (!wgpu_pipeline)
        {
            spdlog::error("WgpuComputePassEncoder::set_pipeline: Invalid pipeline");
            return;
        }

        wgpuComputePassEncoderSetPipeline(_encoder, wgpu_pipeline);
    }

    void WgpuComputePassEncoder::set_bind_group(uint32_t group_index, const render::BindGroup& bind_group, const uint32_t* dynamic_offsets, uint32_t dynamic_offset_count) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuComputePassEncoder::set_bind_group: Invalid encoder");
            return;
        }

        WGPUBindGroup wgpu_bind_group = static_cast<WGPUBindGroup>(bind_group.native_handle());
        if (!wgpu_bind_group)
        {
            spdlog::error("WgpuComputePassEncoder::set_bind_group: Invalid bind group");
            return;
        }

        wgpuComputePassEncoderSetBindGroup(_encoder, group_index, wgpu_bind_group, dynamic_offset_count, dynamic_offsets);
    }

    void WgpuComputePassEncoder::dispatch(uint32_t workgroup_count_x, uint32_t workgroup_count_y, uint32_t workgroup_count_z) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuComputePassEncoder::dispatch: Invalid encoder");
            return;
        }

        wgpuComputePassEncoderDispatchWorkgroups(_encoder, workgroup_count_x, workgroup_count_y, workgroup_count_z);
    }

    void WgpuComputePassEncoder::end() const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuComputePassEncoder::end: Invalid encoder");
            return;
        }

        wgpuComputePassEncoderEnd(_encoder);
        wgpuComputePassEncoderRelease(_encoder);
        _encoder = nullptr; // Mark as ended
    }

} // namespace raktr::render::backend::wgpu
