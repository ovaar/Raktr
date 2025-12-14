/*!
 * @file wgpu_render_pass_encoder.cpp
 * @brief WebGPU render pass encoder implementation.
 */

#include "wgpu_render_pass_encoder.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend
{
    void WgpuRenderPassEncoder::set_vertex_buffer(uint32_t slot, render::Buffer buffer, uint64_t offset, uint64_t size) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuRenderPassEncoder::set_vertex_buffer: Invalid encoder");
            return;
        }

        if (!buffer.is_valid() || buffer.id() >= _buffers->size())
        {
            spdlog::error("WgpuRenderPassEncoder::set_vertex_buffer: Invalid buffer");
            return;
        }

        WGPUBuffer wgpu_buffer = (*_buffers)[buffer.id()];
        if (!wgpu_buffer)
        {
            spdlog::error("WgpuRenderPassEncoder::set_vertex_buffer: Null buffer handle");
            return;
        }

        wgpuRenderPassEncoderSetVertexBuffer(_encoder, slot, wgpu_buffer, offset, size);
    }

    void WgpuRenderPassEncoder::set_index_buffer(render::Buffer buffer, uint64_t offset, uint64_t size) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuRenderPassEncoder::set_index_buffer: Invalid encoder");
            return;
        }

        if (!buffer.is_valid() || buffer.id() >= _buffers->size())
        {
            spdlog::error("WgpuRenderPassEncoder::set_index_buffer: Invalid buffer");
            return;
        }

        WGPUBuffer wgpu_buffer = (*_buffers)[buffer.id()];
        if (!wgpu_buffer)
        {
            spdlog::error("WgpuRenderPassEncoder::set_index_buffer: Null buffer handle");
            return;
        }

        // Assume Uint32 index format (most common)
        wgpuRenderPassEncoderSetIndexBuffer(_encoder, wgpu_buffer, WGPUIndexFormat_Uint32, offset, size);
    }

    void WgpuRenderPassEncoder::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuRenderPassEncoder::draw: Invalid encoder");
            return;
        }

        wgpuRenderPassEncoderDraw(_encoder, vertex_count, instance_count, first_vertex, first_instance);
    }

    void WgpuRenderPassEncoder::draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t base_vertex, uint32_t first_instance) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuRenderPassEncoder::draw_indexed: Invalid encoder");
            return;
        }

        wgpuRenderPassEncoderDrawIndexed(_encoder, index_count, instance_count, first_index, base_vertex, first_instance);
    }

    void WgpuRenderPassEncoder::end() const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuRenderPassEncoder::end: Invalid encoder");
            return;
        }

        wgpuRenderPassEncoderEnd(_encoder);
        wgpuRenderPassEncoderRelease(_encoder);
        _encoder = nullptr; // Mark as ended
    }

} // namespace raktr::render::backend
