/*!
 * @file wgpu_compute_pass_encoder.cpp
 * @brief WebGPU compute pass encoder implementation.
 */

#include "wgpu_compute_pass_encoder.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend
{
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

} // namespace raktr::render::backend
