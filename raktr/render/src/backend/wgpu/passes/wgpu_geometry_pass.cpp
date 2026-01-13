/*!
 * @file geometry_pass.cpp
 * @brief Implementation of WebGPU geometry rendering pass.
 */

#include "wgpu_geometry_pass.h"
#include "backend/wgpu/wgpu_render_pass_builder.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend::wgpu
{

    WgpuGeometryPass::WgpuGeometryPass(
        Buffer                   vertex_buffer,
        Buffer                   index_buffer,
        Buffer                   instance_buffer,
        const std::vector<bool>* visibility,
        WGPURenderPipeline       render_pipeline,
        uint32_t                 instance_count,
        uint32_t                 index_count)
        : _vertex_buffer(vertex_buffer), _index_buffer(index_buffer), _instance_buffer(instance_buffer), _visibility(visibility), _render_pipeline(render_pipeline), _instance_count(instance_count), _index_count(index_count)
    {
    }

    void WgpuGeometryPass::execute(WgpuPassContext& ctx)
    {
        // Build render pass descriptor
        WgpuRenderPassBuilder builder;
        builder.color_attachment(ctx.color_target, WGPULoadOp_Clear, { 0.1f, 0.1f, 0.15f, 1.0f })
            .depth_attachment(ctx.depth_target, WGPULoadOp_Clear, 1.0f)
            .label("WgpuGeometryPass");

        WGPURenderPassEncoder pass = builder.begin(ctx.command_encoder);

        // Set pipeline and buffers
        wgpuRenderPassEncoderSetPipeline(pass, _render_pipeline);
        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, reinterpret_cast<WGPUBuffer>(_vertex_buffer.id()), 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetVertexBuffer(pass, 1, reinterpret_cast<WGPUBuffer>(_instance_buffer.id()), 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(pass,
                                            reinterpret_cast<WGPUBuffer>(_index_buffer.id()),
                                            WGPUIndexFormat_Uint32,
                                            0,
                                            WGPU_WHOLE_SIZE);

        // Draw visible instances only
        uint32_t drawn_count = 0;
        for (uint32_t i = 0; i < _instance_count; ++i)
        {
            // Check visibility (with bounds check)
            bool is_visible = (i < _visibility->size()) ? (*_visibility)[i] : true;

            if (is_visible)
            {
                // Draw this instance
                wgpuRenderPassEncoderDrawIndexed(
                    pass,
                    _index_count, // indexCount
                    1,            // instanceCount (1 instance at a time)
                    0,            // firstIndex
                    0,            // baseVertex
                    i             // firstInstance (use i to index into instance buffer)
                );
                drawn_count++;
            }
        }

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        // Log draw statistics
        float culling_rate = _instance_count > 0
                                 ? (100.0f * (_instance_count - drawn_count) / _instance_count)
                                 : 0.0f;

        spdlog::info("WgpuGeometryPass: Drew {}/{} instances ({:.1f}% culled)",
                     drawn_count,
                     _instance_count,
                     culling_rate);
    }

    void WgpuGeometryPass::on_viewport_resize([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
    {
        // Nothing to do - render targets are managed by WgpuPassContext
    }

    void WgpuGeometryPass::update_visibility([[maybe_unused]] const std::vector<bool>* visibility)
    {
        // Note: We're storing a const reference, so the caller must ensure
        // the visibility vector outlives this pass or is updated before execute()
        // In practice, this will be managed by the render graph
    }

} // namespace raktr::render::backend::wgpu
