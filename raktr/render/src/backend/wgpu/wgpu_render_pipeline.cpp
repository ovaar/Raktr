/*!
 * @file wgpu_render_pipeline.cpp
 * @brief WebGPU render pipeline implementation.
 */

#include "wgpu_render_pipeline.h"
#include "wgpu_shader_module.h"
#include <cstddef>
#include <spdlog/spdlog.h>
#include <vector>

namespace raktr::render::backend::wgpu
{

    namespace
    {

        // Convert VertexFormat to WebGPU
        WGPUVertexFormat convert_vertex_format(VertexFormat format)
        {
            switch (format)
            {
                case VertexFormat::Float32x2:
                    return WGPUVertexFormat_Float32x2;
                case VertexFormat::Float32x3:
                    return WGPUVertexFormat_Float32x3;
                case VertexFormat::Float32x4:
                    return WGPUVertexFormat_Float32x4;
                case VertexFormat::Uint32:
                    return WGPUVertexFormat_Uint32;
                case VertexFormat::Sint32:
                    return WGPUVertexFormat_Sint32;
                default:
                    return WGPUVertexFormat_Float32x3;
            }
        }

        WGPUVertexStepMode convert_step_mode(VertexStepMode mode)
        {
            return mode == VertexStepMode::Vertex ? WGPUVertexStepMode_Vertex : WGPUVertexStepMode_Instance;
        }

        WGPUPrimitiveTopology convert_topology(PrimitiveTopology topology)
        {
            switch (topology)
            {
                case PrimitiveTopology::PointList:
                    return WGPUPrimitiveTopology_PointList;
                case PrimitiveTopology::LineList:
                    return WGPUPrimitiveTopology_LineList;
                case PrimitiveTopology::LineStrip:
                    return WGPUPrimitiveTopology_LineStrip;
                case PrimitiveTopology::TriangleList:
                    return WGPUPrimitiveTopology_TriangleList;
                case PrimitiveTopology::TriangleStrip:
                    return WGPUPrimitiveTopology_TriangleStrip;
                default:
                    return WGPUPrimitiveTopology_TriangleList;
            }
        }

        WGPUFrontFace convert_front_face(FrontFace face)
        {
            return face == FrontFace::CCW ? WGPUFrontFace_CCW : WGPUFrontFace_CW;
        }

        WGPUCullMode convert_cull_mode(CullMode mode)
        {
            switch (mode)
            {
                case CullMode::None:
                    return WGPUCullMode_None;
                case CullMode::Front:
                    return WGPUCullMode_Front;
                case CullMode::Back:
                    return WGPUCullMode_Back;
                default:
                    return WGPUCullMode_None;
            }
        }

        WGPUBlendOperation convert_blend_op(BlendOperation op)
        {
            switch (op)
            {
                case BlendOperation::Add:
                    return WGPUBlendOperation_Add;
                case BlendOperation::Subtract:
                    return WGPUBlendOperation_Subtract;
                case BlendOperation::ReverseSubtract:
                    return WGPUBlendOperation_ReverseSubtract;
                case BlendOperation::Min:
                    return WGPUBlendOperation_Min;
                case BlendOperation::Max:
                    return WGPUBlendOperation_Max;
                default:
                    return WGPUBlendOperation_Add;
            }
        }

        WGPUBlendFactor convert_blend_factor(BlendFactor factor)
        {
            switch (factor)
            {
                case BlendFactor::Zero:
                    return WGPUBlendFactor_Zero;
                case BlendFactor::One:
                    return WGPUBlendFactor_One;
                case BlendFactor::Src:
                    return WGPUBlendFactor_Src;
                case BlendFactor::OneMinusSrc:
                    return WGPUBlendFactor_OneMinusSrc;
                case BlendFactor::SrcAlpha:
                    return WGPUBlendFactor_SrcAlpha;
                case BlendFactor::OneMinusSrcAlpha:
                    return WGPUBlendFactor_OneMinusSrcAlpha;
                case BlendFactor::Dst:
                    return WGPUBlendFactor_Dst;
                case BlendFactor::OneMinusDst:
                    return WGPUBlendFactor_OneMinusDst;
                case BlendFactor::DstAlpha:
                    return WGPUBlendFactor_DstAlpha;
                case BlendFactor::OneMinusDstAlpha:
                    return WGPUBlendFactor_OneMinusDstAlpha;
                case BlendFactor::SrcAlphaSaturated:
                    return WGPUBlendFactor_SrcAlphaSaturated;
                case BlendFactor::Constant:
                    return WGPUBlendFactor_Constant;
                case BlendFactor::OneMinusConstant:
                    return WGPUBlendFactor_OneMinusConstant;
                default:
                    return WGPUBlendFactor_One;
            }
        }

        WGPUCompareFunction convert_compare_function(CompareFunction func)
        {
            switch (func)
            {
                case CompareFunction::Never:
                    return WGPUCompareFunction_Never;
                case CompareFunction::Less:
                    return WGPUCompareFunction_Less;
                case CompareFunction::Equal:
                    return WGPUCompareFunction_Equal;
                case CompareFunction::LessEqual:
                    return WGPUCompareFunction_LessEqual;
                case CompareFunction::Greater:
                    return WGPUCompareFunction_Greater;
                case CompareFunction::NotEqual:
                    return WGPUCompareFunction_NotEqual;
                case CompareFunction::GreaterEqual:
                    return WGPUCompareFunction_GreaterEqual;
                case CompareFunction::Always:
                    return WGPUCompareFunction_Always;
                default:
                    return WGPUCompareFunction_Always;
            }
        }

        WGPUStencilOperation convert_stencil_op(StencilOperation op)
        {
            switch (op)
            {
                case StencilOperation::Keep:
                    return WGPUStencilOperation_Keep;
                case StencilOperation::Zero:
                    return WGPUStencilOperation_Zero;
                case StencilOperation::Replace:
                    return WGPUStencilOperation_Replace;
                case StencilOperation::Invert:
                    return WGPUStencilOperation_Invert;
                case StencilOperation::IncrementClamp:
                    return WGPUStencilOperation_IncrementClamp;
                case StencilOperation::DecrementClamp:
                    return WGPUStencilOperation_DecrementClamp;
                case StencilOperation::IncrementWrap:
                    return WGPUStencilOperation_IncrementWrap;
                case StencilOperation::DecrementWrap:
                    return WGPUStencilOperation_DecrementWrap;
                default:
                    return WGPUStencilOperation_Keep;
            }
        }

        WGPUTextureFormat convert_texture_format(TextureFormat format)
        {
            switch (format)
            {
                case TextureFormat::BGRA8Unorm:
                    return WGPUTextureFormat_BGRA8Unorm;
                case TextureFormat::RGBA8Unorm:
                    return WGPUTextureFormat_RGBA8Unorm;
                case TextureFormat::Depth24PlusStencil8:
                    return WGPUTextureFormat_Depth24PlusStencil8;
                case TextureFormat::Depth32Float:
                    return WGPUTextureFormat_Depth32Float;
                default:
                    return WGPUTextureFormat_BGRA8Unorm;
            }
        }

        WGPUColorWriteMask convert_color_write_mask(ColorWriteMask mask)
        {
            return static_cast<WGPUColorWriteMask>(static_cast<uint8_t>(mask));
        }
    } // namespace

    WgpuRenderPipeline::WgpuRenderPipeline(WGPUDevice device, const RenderPipelineDescriptor& descriptor)
    {
        // Convert vertex buffer layouts
        std::vector<WGPUVertexBufferLayout>           vertex_buffers;
        std::vector<std::vector<WGPUVertexAttribute>> vertex_attributes_storage;

        vertex_buffers.reserve(descriptor.vertex_buffers.size());
        vertex_attributes_storage.reserve(descriptor.vertex_buffers.size());

        for (const auto& buffer_layout : descriptor.vertex_buffers)
        {
            std::vector<WGPUVertexAttribute> attributes;
            attributes.reserve(buffer_layout.attributes.size());

            for (const auto& attr : buffer_layout.attributes)
            {
                WGPUVertexAttribute wgpu_attr = {};
                wgpu_attr.format              = convert_vertex_format(attr.format);
                wgpu_attr.offset              = attr.offset;
                wgpu_attr.shaderLocation      = attr.shader_location;
                attributes.push_back(wgpu_attr);
            }

            vertex_attributes_storage.push_back(std::move(attributes));
        }

        for (size_t i = 0; i < descriptor.vertex_buffers.size(); ++i)
        {
            WGPUVertexBufferLayout buffer = {};
            buffer.arrayStride            = descriptor.vertex_buffers[i].array_stride;
            buffer.stepMode               = convert_step_mode(descriptor.vertex_buffers[i].step_mode);
            buffer.attributeCount         = static_cast<uint32_t>(vertex_attributes_storage[i].size());
            buffer.attributes             = vertex_attributes_storage[i].data();
            vertex_buffers.push_back(buffer);
        }

        // Vertex state
        WGPUVertexState vertex_state = {};
        vertex_state.module          = static_cast<WGPUShaderModule>(descriptor.vertex_shader.native_handle());
        vertex_state.entryPoint      = WGPUStringView{ descriptor.vertex_entry_point.data(), descriptor.vertex_entry_point.length() };
        vertex_state.bufferCount     = static_cast<uint32_t>(vertex_buffers.size());
        vertex_state.buffers         = vertex_buffers.empty() ? nullptr : vertex_buffers.data();

        // Primitive state
        WGPUPrimitiveState primitive_state = {};
        primitive_state.topology           = convert_topology(descriptor.primitive.topology);
        primitive_state.frontFace          = convert_front_face(descriptor.primitive.front_face);
        primitive_state.cullMode           = convert_cull_mode(descriptor.primitive.cull_mode);

        // Fragment state
        std::vector<WGPUColorTargetState> color_targets;
        std::vector<WGPUBlendState>       blend_states;

        color_targets.reserve(descriptor.color_targets.size());
        blend_states.reserve(descriptor.color_targets.size());

        for (const auto& target : descriptor.color_targets)
        {
            WGPUColorTargetState wgpu_target = {};
            wgpu_target.format               = convert_texture_format(target.format);

            if (target.blend.has_value())
            {
                WGPUBlendState blend  = {};
                blend.color.operation = convert_blend_op(target.blend->color.operation);
                blend.color.srcFactor = convert_blend_factor(target.blend->color.src_factor);
                blend.color.dstFactor = convert_blend_factor(target.blend->color.dst_factor);
                blend.alpha.operation = convert_blend_op(target.blend->alpha.operation);
                blend.alpha.srcFactor = convert_blend_factor(target.blend->alpha.src_factor);
                blend.alpha.dstFactor = convert_blend_factor(target.blend->alpha.dst_factor);
                blend_states.push_back(blend);
                wgpu_target.blend     = &blend_states.back();
                wgpu_target.writeMask = convert_color_write_mask(target.blend->write_mask);
            }
            else
            {
                wgpu_target.blend     = nullptr;
                wgpu_target.writeMask = WGPUColorWriteMask_All;
            }

            color_targets.push_back(wgpu_target);
        }

        WGPUFragmentState fragment_state = {};
        fragment_state.module            = static_cast<WGPUShaderModule>(descriptor.fragment_shader.native_handle());
        fragment_state.entryPoint        = WGPUStringView{ descriptor.fragment_entry_point.data(), descriptor.fragment_entry_point.length() };
        fragment_state.targetCount       = static_cast<uint32_t>(color_targets.size());
        fragment_state.targets           = color_targets.data();

        // Depth/stencil state
        WGPUDepthStencilState  depth_stencil_state = {};
        WGPUDepthStencilState* depth_stencil_ptr   = nullptr;

        if (descriptor.depth_stencil.has_value())
        {
            const auto& ds                               = descriptor.depth_stencil.value();
            depth_stencil_state.format                   = WGPUTextureFormat_Depth24PlusStencil8; // Could be made configurable
            depth_stencil_state.depthWriteEnabled        = ds.depth_write_enabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;
            depth_stencil_state.depthCompare             = convert_compare_function(ds.depth_compare);
            depth_stencil_state.stencilFront.compare     = convert_compare_function(ds.stencil_front.compare);
            depth_stencil_state.stencilFront.failOp      = convert_stencil_op(ds.stencil_front.fail_op);
            depth_stencil_state.stencilFront.depthFailOp = convert_stencil_op(ds.stencil_front.depth_fail_op);
            depth_stencil_state.stencilFront.passOp      = convert_stencil_op(ds.stencil_front.pass_op);
            depth_stencil_state.stencilBack.compare      = convert_compare_function(ds.stencil_back.compare);
            depth_stencil_state.stencilBack.failOp       = convert_stencil_op(ds.stencil_back.fail_op);
            depth_stencil_state.stencilBack.depthFailOp  = convert_stencil_op(ds.stencil_back.depth_fail_op);
            depth_stencil_state.stencilBack.passOp       = convert_stencil_op(ds.stencil_back.pass_op);
            depth_stencil_state.stencilReadMask          = ds.stencil_read_mask;
            depth_stencil_state.stencilWriteMask         = ds.stencil_write_mask;
            depth_stencil_ptr                            = &depth_stencil_state;
        }

        // Create pipeline
        WGPURenderPipelineDescriptor pipeline_desc = {};
        pipeline_desc.label                        = descriptor.label.empty() ? WGPUStringView{ nullptr, 0 } : WGPUStringView{ descriptor.label.data(), descriptor.label.length() };
        pipeline_desc.vertex                       = vertex_state;
        pipeline_desc.primitive                    = primitive_state;
        pipeline_desc.fragment                     = &fragment_state;
        pipeline_desc.depthStencil                 = depth_stencil_ptr;
        pipeline_desc.multisample.count            = 1;
        pipeline_desc.multisample.mask             = 0xFFFFFFFF;

        _pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);

        if (!_pipeline)
        {
            spdlog::error("Failed to create render pipeline: {}", descriptor.label);
        }
    }

    WgpuRenderPipeline::~WgpuRenderPipeline()
    {
        if (_pipeline)
        {
            wgpuRenderPipelineRelease(_pipeline);
            _pipeline = nullptr;
        }
    }

    WgpuRenderPipeline::WgpuRenderPipeline(WgpuRenderPipeline&& other) noexcept
        : _pipeline(other._pipeline)
    {
        other._pipeline = nullptr;
    }

    WgpuRenderPipeline& WgpuRenderPipeline::operator=(WgpuRenderPipeline&& other) noexcept
    {
        if (this != &other)
        {
            if (_pipeline)
            {
                wgpuRenderPipelineRelease(_pipeline);
            }
            _pipeline       = other._pipeline;
            other._pipeline = nullptr;
        }
        return *this;
    }

} // namespace raktr::render::backend::wgpu
