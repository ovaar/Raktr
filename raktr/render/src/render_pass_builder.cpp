/*!
 * @file render_pass_builder.cpp
 * @brief Implementation of RenderPassBuilder fluent API.
 */

#include "render_pass_builder.h"
#include <webgpu/webgpu.h>

namespace raktr::render
{

    RenderPassBuilder& RenderPassBuilder::color_attachment(
        WGPUTextureView      target,
        WGPULoadOp           load_op,
        std::array<float, 4> clear_color)
    {
        _color = ColorAttachmentDesc{
            .view        = target,
            .load_op     = load_op,
            .clear_color = clear_color
        };
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::depth_attachment(
        WGPUTextureView target,
        WGPULoadOp      load_op,
        float           clear_depth)
    {
        _depth = DepthAttachmentDesc{
            .view        = target,
            .load_op     = load_op,
            .clear_depth = clear_depth
        };
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::label(std::string_view name)
    {
        _label = name;
        return *this;
    }

    WGPURenderPassEncoder RenderPassBuilder::begin(WGPUCommandEncoder encoder)
    {
        WGPURenderPassDescriptor render_pass_desc = {};
        render_pass_desc.nextInChain              = nullptr;

        WGPUStringView label_view = {};
        if (!_label.empty())
        {
            label_view.data   = _label.c_str();
            label_view.length = _label.size();
        }
        render_pass_desc.label = label_view;

        // Color attachment
        WGPURenderPassColorAttachment color_attachment = {};
        if (_color.has_value())
        {
            color_attachment.view          = _color->view;
            color_attachment.depthSlice    = WGPU_DEPTH_SLICE_UNDEFINED;
            color_attachment.resolveTarget = nullptr;
            color_attachment.loadOp        = static_cast<WGPULoadOp>(_color->load_op);
            color_attachment.storeOp       = WGPUStoreOp_Store;
            color_attachment.clearValue    = {
                _color->clear_color[0],
                _color->clear_color[1],
                _color->clear_color[2],
                _color->clear_color[3]
            };

            render_pass_desc.colorAttachmentCount = 1;
            render_pass_desc.colorAttachments     = &color_attachment;
        }
        else
        {
            render_pass_desc.colorAttachmentCount = 0;
            render_pass_desc.colorAttachments     = nullptr;
        }

        // Depth attachment
        WGPURenderPassDepthStencilAttachment depth_attachment = {};
        if (_depth.has_value())
        {
            depth_attachment.view              = _depth->view;
            depth_attachment.depthClearValue   = _depth->clear_depth;
            depth_attachment.depthLoadOp       = static_cast<WGPULoadOp>(_depth->load_op);
            depth_attachment.depthStoreOp      = WGPUStoreOp_Store;
            depth_attachment.depthReadOnly     = false;
            depth_attachment.stencilClearValue = 0;
            depth_attachment.stencilLoadOp     = WGPULoadOp_Load;
            depth_attachment.stencilStoreOp    = WGPUStoreOp_Undefined;
            depth_attachment.stencilReadOnly   = false;

            render_pass_desc.depthStencilAttachment = &depth_attachment;
        }
        else
        {
            render_pass_desc.depthStencilAttachment = nullptr;
        }

        render_pass_desc.timestampWrites   = nullptr;
        render_pass_desc.occlusionQuerySet = nullptr;

        return wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
    }

} // namespace raktr::render
