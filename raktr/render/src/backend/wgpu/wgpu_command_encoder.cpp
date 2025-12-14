/*!
 * @file wgpu_command_encoder.cpp
 * @brief WebGPU command encoder implementation.
 */

#include "wgpu_command_encoder.h"
#include "wgpu_compute_pass_encoder.h"
#include "wgpu_render_pass_encoder.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend
{
    render::RenderPassEncoder WgpuCommandEncoder::begin_render_pass(const render::RenderPassDescriptor& descriptor) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuCommandEncoder::begin_render_pass: Invalid encoder");
            return render::RenderPassEncoder(WgpuRenderPassEncoder(nullptr, nullptr));
        }

        WGPURenderPassDescriptor wgpu_desc = {};
        wgpu_desc.nextInChain              = nullptr;
        wgpu_desc.label                    = { "Render Pass", WGPU_STRLEN };

        // Convert color attachments
        std::array<WGPURenderPassColorAttachment, 8> wgpu_color_attachments = {};
        for (uint32_t i = 0; i < descriptor.color_attachment_count && i < 8; ++i)
        {
            const auto& att                         = descriptor.color_attachments[i];
            wgpu_color_attachments[i].view          = static_cast<WGPUTextureView>(att.view);
            wgpu_color_attachments[i].resolveTarget = static_cast<WGPUTextureView>(att.resolve);
            wgpu_color_attachments[i].loadOp        = att.load_op == render::LoadOp::Load ? WGPULoadOp_Load : WGPULoadOp_Clear;
            wgpu_color_attachments[i].storeOp       = att.store_op == render::StoreOp::Store ? WGPUStoreOp_Store : WGPUStoreOp_Discard;
            wgpu_color_attachments[i].clearValue.r  = att.clear_color.r;
            wgpu_color_attachments[i].clearValue.g  = att.clear_color.g;
            wgpu_color_attachments[i].clearValue.b  = att.clear_color.b;
            wgpu_color_attachments[i].clearValue.a  = att.clear_color.a;
        }

        wgpu_desc.colorAttachmentCount = descriptor.color_attachment_count;
        wgpu_desc.colorAttachments     = wgpu_color_attachments.data();

        // Convert depth/stencil attachment
        WGPURenderPassDepthStencilAttachment wgpu_depth_stencil = {};
        if (descriptor.depth_stencil_attachment.has_value())
        {
            const auto& ds_att                   = descriptor.depth_stencil_attachment.value();
            wgpu_depth_stencil.view              = static_cast<WGPUTextureView>(ds_att.view);
            wgpu_depth_stencil.depthLoadOp       = ds_att.depth_load_op == render::LoadOp::Load ? WGPULoadOp_Load : WGPULoadOp_Clear;
            wgpu_depth_stencil.depthStoreOp      = ds_att.depth_store_op == render::StoreOp::Store ? WGPUStoreOp_Store : WGPUStoreOp_Discard;
            wgpu_depth_stencil.depthClearValue   = ds_att.depth_clear_value;
            wgpu_depth_stencil.depthReadOnly     = ds_att.depth_read_only;
            wgpu_depth_stencil.stencilLoadOp     = ds_att.stencil_load_op == render::LoadOp::Load ? WGPULoadOp_Load : WGPULoadOp_Clear;
            wgpu_depth_stencil.stencilStoreOp    = ds_att.stencil_store_op == render::StoreOp::Store ? WGPUStoreOp_Store : WGPUStoreOp_Discard;
            wgpu_depth_stencil.stencilClearValue = ds_att.stencil_clear_value;
            wgpu_depth_stencil.stencilReadOnly   = ds_att.stencil_read_only;
            wgpu_desc.depthStencilAttachment     = &wgpu_depth_stencil;
        }
        else
        {
            wgpu_desc.depthStencilAttachment = nullptr;
        }

        WGPURenderPassEncoder wgpu_render_pass = wgpuCommandEncoderBeginRenderPass(_encoder, &wgpu_desc);

        if (!wgpu_render_pass)
        {
            spdlog::error("WgpuCommandEncoder::begin_render_pass: Failed to create render pass");
            return render::RenderPassEncoder(WgpuRenderPassEncoder(nullptr, nullptr));
        }

        spdlog::debug("WgpuCommandEncoder: Began render pass with {} color attachments", descriptor.color_attachment_count);

        return render::RenderPassEncoder(WgpuRenderPassEncoder(wgpu_render_pass, _buffers));
    }

    render::ComputePassEncoder WgpuCommandEncoder::begin_compute_pass(const render::ComputePassDescriptor& descriptor) const
    {
        (void)descriptor; // Currently unused - future: timestamp queries, labels

        if (!_encoder)
        {
            spdlog::error("WgpuCommandEncoder::begin_compute_pass: Invalid encoder");
            return render::ComputePassEncoder(WgpuComputePassEncoder(nullptr));
        }

        WGPUComputePassDescriptor wgpu_desc = {};
        wgpu_desc.nextInChain               = nullptr;
        wgpu_desc.label                     = { "Compute Pass", WGPU_STRLEN };

        WGPUComputePassEncoder wgpu_compute_pass = wgpuCommandEncoderBeginComputePass(_encoder, &wgpu_desc);

        if (!wgpu_compute_pass)
        {
            spdlog::error("WgpuCommandEncoder::begin_compute_pass: Failed to create compute pass");
            return render::ComputePassEncoder(WgpuComputePassEncoder(nullptr));
        }

        spdlog::debug("WgpuCommandEncoder: Began compute pass");

        return render::ComputePassEncoder(WgpuComputePassEncoder(wgpu_compute_pass));
    }

    void WgpuCommandEncoder::copy_buffer_to_buffer(render::Buffer source, uint64_t source_offset, render::Buffer destination, uint64_t destination_offset, uint64_t size) const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuCommandEncoder::copy_buffer_to_buffer: Invalid encoder");
            return;
        }

        if (!source.is_valid() || !destination.is_valid())
        {
            spdlog::error("WgpuCommandEncoder::copy_buffer_to_buffer: Invalid buffer");
            return;
        }

        if (source.id() >= _buffers->size() || destination.id() >= _buffers->size())
        {
            spdlog::error("WgpuCommandEncoder::copy_buffer_to_buffer: Buffer ID out of range");
            return;
        }

        WGPUBuffer src_buffer = (*_buffers)[source.id()];
        WGPUBuffer dst_buffer = (*_buffers)[destination.id()];

        if (!src_buffer || !dst_buffer)
        {
            spdlog::error("WgpuCommandEncoder::copy_buffer_to_buffer: Null buffer handle");
            return;
        }

        wgpuCommandEncoderCopyBufferToBuffer(_encoder,
                                             src_buffer,
                                             source_offset,
                                             dst_buffer,
                                             destination_offset,
                                             size);

        spdlog::debug("WgpuCommandEncoder: Recorded buffer copy {} bytes from buffer {} to buffer {}",
                      size,
                      source.id(),
                      destination.id());
    }

    render::CommandBuffer WgpuCommandEncoder::finish() const
    {
        if (!_encoder)
        {
            spdlog::error("WgpuCommandEncoder::finish: Invalid encoder");
            return render::CommandBuffer();
        }

        WGPUCommandBufferDescriptor desc = {};
        desc.nextInChain                 = nullptr;
        desc.label                       = { "Command Buffer", WGPU_STRLEN };

        WGPUCommandBuffer wgpu_cmd_buffer = wgpuCommandEncoderFinish(_encoder, &desc);

        if (!wgpu_cmd_buffer)
        {
            spdlog::error("WgpuCommandEncoder::finish: Failed to finish command encoder");
            return render::CommandBuffer();
        }

        // Register command buffer in device storage
        _command_buffers->push_back(wgpu_cmd_buffer);
        uint64_t cmd_buffer_id = _command_buffers->size() - 1;

        spdlog::debug("WgpuCommandEncoder: Finished command buffer with ID {}", cmd_buffer_id);

        // Release encoder (marks it as finished)
        wgpuCommandEncoderRelease(_encoder);
        _encoder = nullptr;

        return render::CommandBuffer(cmd_buffer_id);
    }

} // namespace raktr::render::backend
