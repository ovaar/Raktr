/*!
 * @file wgpu_render_pass_encoder.cpp
 * @brief WebGPU render pass encoder implementation.
 */

#include "wgpu_render_pass_encoder.h"
#include <spdlog/spdlog.h>
namespace raktr::render::backend::wgpu
{
    void WgpuRenderPassEncoder::set_pipeline(const render::RenderPipeline& pipeline) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuRenderPassEncoder::set_pipeline: Invalid encoder");
            return;
        }

        WGPURenderPipeline wgpu_pipeline = static_cast<WGPURenderPipeline>(pipeline.native_handle());
        if (!wgpu_pipeline)
        {
            spdlog::error("WgpuRenderPassEncoder::set_pipeline: Invalid pipeline");
            return;
        }

        wgpuRenderPassEncoderSetPipeline(_encoder, wgpu_pipeline);
    }

    void WgpuRenderPassEncoder::set_bind_group(uint32_t group_index, const render::BindGroup& bind_group, const uint32_t* dynamic_offsets, uint32_t dynamic_offset_count) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuRenderPassEncoder::set_bind_group: Invalid encoder");
            return;
        }

        WGPUBindGroup wgpu_bind_group = static_cast<WGPUBindGroup>(bind_group.native_handle());
        if (!wgpu_bind_group)
        {
            spdlog::error("WgpuRenderPassEncoder::set_bind_group: Invalid bind group");
            return;
        }

        wgpuRenderPassEncoderSetBindGroup(_encoder, group_index, wgpu_bind_group, dynamic_offset_count, dynamic_offsets);
    }

    void WgpuRenderPassEncoder::set_viewport(float x, float y, float width, float height, float min_depth, float max_depth) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuRenderPassEncoder::set_viewport: Invalid encoder");
            return;
        }

        wgpuRenderPassEncoderSetViewport(_encoder, x, y, width, height, min_depth, max_depth);
    }

    void WgpuRenderPassEncoder::set_scissor_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuRenderPassEncoder::set_scissor_rect: Invalid encoder");
            return;
        }

        wgpuRenderPassEncoderSetScissorRect(_encoder, x, y, width, height);
    }

    void WgpuRenderPassEncoder::set_blend_constant(float r, float g, float b, float a) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuRenderPassEncoder::set_blend_constant: Invalid encoder");
            return;
        }

        WGPUColor color = { r, g, b, a };
        wgpuRenderPassEncoderSetBlendConstant(_encoder, &color);
    }

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

} // namespace raktr::render::backend::wgpu
