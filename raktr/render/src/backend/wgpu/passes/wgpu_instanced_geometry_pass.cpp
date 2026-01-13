/*!
 * @file instanced_geometry_pass.cpp
 * @brief WebGPU instanced geometry rendering pass implementation.
 */

#include "wgpu_instanced_geometry_pass.h"
#include "backend/wgpu/wgpu_device.h"
#include "backend/wgpu/wgpu_pass_context.h"
#include "render_pass_builder.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend::wgpu
{

    WgpuInstancedGeometryPass::WgpuInstancedGeometryPass(WgpuDevice*                      device,
                                                         Buffer                           vertex_buffer,
                                                         Buffer                           index_buffer,
                                                         Buffer                           instance_buffer,
                                                         const std::vector<InstanceData>* all_instances,
                                                         const std::vector<bool>*         visibility,
                                                         uint32_t                         index_count)
        : _device(device),
          _vertex_buffer(vertex_buffer),
          _index_buffer(index_buffer),
          _instance_buffer(instance_buffer),
          _all_instances(all_instances),
          _visibility(visibility),
          _index_count(index_count),
          _last_drawn_count(0)
    {
    }

    void WgpuInstancedGeometryPass::execute(WgpuPassContext& ctx)
    {
        // Build instance data for visible objects only
        std::vector<InstanceData> visible_instances;
        visible_instances.reserve(_all_instances->size());

        for (size_t i = 0; i < _all_instances->size(); ++i)
        {
            // Check visibility (with bounds check)
            bool is_visible = (i < _visibility->size()) ? (*_visibility)[i] : true;

            if (is_visible)
            {
                visible_instances.push_back((*_all_instances)[i]);
            }
        }

        _last_drawn_count = static_cast<uint32_t>(visible_instances.size());

        // Update instance buffer with visible instances
        if (visible_instances.empty())
        {
            return; // Nothing to draw
        }

        auto update_result = _device->update_instance_buffer(
            _instance_buffer,
            std::as_bytes(std::span(visible_instances)));

        if (!update_result.has_value())
        {
            spdlog::error("WgpuInstancedGeometryPass: Failed to update instance buffer");
            return;
        }

        // Build render pass using PassContext's command encoder and targets
        RenderPassBuilder builder;
        builder.color_attachment(ctx.color_target, WGPULoadOp_Clear, { 0.1f, 0.2f, 0.3f, 1.0f })
            .depth_attachment(ctx.depth_target, WGPULoadOp_Clear, 1.0f)
            .label("WgpuInstancedGeometryPass");

        WGPURenderPassEncoder pass = builder.begin(ctx.command_encoder);

        // Set pipeline and bind group (get from device)
        wgpuRenderPassEncoderSetPipeline(pass, _device->wgpu_render_pipeline());
        wgpuRenderPassEncoderSetBindGroup(pass, 0, _device->wgpu_current_bind_group(), 0, nullptr);

        // Set viewport to maintain aspect ratio (use full context viewport)
        wgpuRenderPassEncoderSetViewport(pass,
                                         0.0f,
                                         0.0f,
                                         static_cast<float>(ctx.viewport_width),
                                         static_cast<float>(ctx.viewport_height),
                                         0.0f,
                                         1.0f);

        // Set scissor rect
        wgpuRenderPassEncoderSetScissorRect(pass, 0, 0, ctx.viewport_width, ctx.viewport_height);

        // Bind vertex, index, and instance buffers
        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, reinterpret_cast<WGPUBuffer>(_vertex_buffer.id()), 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetVertexBuffer(pass, 1, reinterpret_cast<WGPUBuffer>(_instance_buffer.id()), 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(pass,
                                            reinterpret_cast<WGPUBuffer>(_index_buffer.id()),
                                            WGPUIndexFormat_Uint32,
                                            0,
                                            WGPU_WHOLE_SIZE);

        // Draw all visible instances in one call (they're already filtered in the instance buffer)
        wgpuRenderPassEncoderDrawIndexed(
            pass,
            _index_count,      // indexCount
            _last_drawn_count, // instanceCount (number of visible instances)
            0,                 // firstIndex
            0,                 // baseVertex
            0                  // firstInstance
        );

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        // Calculate and log culling statistics
        uint32_t total_count  = static_cast<uint32_t>(_all_instances->size());
        uint32_t culled_count = total_count - _last_drawn_count;
        float    culling_rate = total_count > 0
                                    ? (100.0f * culled_count / total_count)
                                    : 0.0f;

        spdlog::info("WgpuInstancedGeometryPass: Drew {}/{} instances ({:.1f}% culled)",
                     _last_drawn_count,
                     total_count,
                     culling_rate);
    }

    void WgpuInstancedGeometryPass::on_viewport_resize([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
    {
        // Nothing to do - render targets are managed by caller
    }

} // namespace raktr::render::backend::wgpu
