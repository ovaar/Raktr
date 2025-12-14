/*!
 * @file test_phase3_api.cpp
 * @brief Tests for Phase 3 API (pipelines, bind groups, shader modules).
 *
 * Tests the public API structure for Phase 3 components.
 * Note: Backend implementations are not yet complete, so these tests focus on API design.
 */

#include "bind_group.h"
#include "compute_pipeline.h"
#include "render_pipeline.h"
#include "shader_module.h"

#include <gtest/gtest.h>

using namespace raktr::render;

// ============================================================================
// Shader Module Tests
// ============================================================================

TEST(Phase3_ShaderModule, DescriptorCreation)
{
    ShaderModuleDescriptor desc;
    desc.code  = R"(
        @vertex
        fn vs_main(@location(0) position: vec3f) -> @builtin(position) vec4f {
            return vec4f(position, 1.0);
        }
    )";
    desc.label = "TestShader";

    EXPECT_FALSE(desc.code.empty());
    EXPECT_EQ(desc.label, "TestShader");
}

TEST(Phase3_ShaderModule, DefaultConstruction)
{
    ShaderModule shader;
    EXPECT_FALSE(shader.is_valid());
    EXPECT_FALSE(static_cast<bool>(shader));
}

// ============================================================================
// Render Pipeline Tests
// ============================================================================

TEST(Phase3_RenderPipeline, VertexFormatEnums)
{
    EXPECT_EQ(static_cast<uint8_t>(VertexFormat::Float32x2), 0);
    EXPECT_EQ(static_cast<uint8_t>(VertexFormat::Float32x3), 1);
    EXPECT_EQ(static_cast<uint8_t>(VertexFormat::Float32x4), 2);
    EXPECT_EQ(static_cast<uint8_t>(VertexFormat::Uint32), 3);
    EXPECT_EQ(static_cast<uint8_t>(VertexFormat::Sint32), 4);
}

TEST(Phase3_RenderPipeline, VertexAttributeCreation)
{
    VertexAttribute attr;
    attr.format          = VertexFormat::Float32x3;
    attr.offset          = 0;
    attr.shader_location = 0;

    EXPECT_EQ(attr.format, VertexFormat::Float32x3);
    EXPECT_EQ(attr.offset, 0);
    EXPECT_EQ(attr.shader_location, 0);
}

TEST(Phase3_RenderPipeline, VertexBufferLayoutCreation)
{
    VertexBufferLayout layout;
    layout.array_stride = 12; // 3 floats * 4 bytes
    layout.step_mode    = VertexStepMode::Vertex;
    layout.attributes.push_back({ .format          = VertexFormat::Float32x3,
                                  .offset          = 0,
                                  .shader_location = 0 });

    EXPECT_EQ(layout.array_stride, 12);
    EXPECT_EQ(layout.step_mode, VertexStepMode::Vertex);
    EXPECT_EQ(layout.attributes.size(), 1);
}

TEST(Phase3_RenderPipeline, PrimitiveTopologyEnums)
{
    EXPECT_EQ(static_cast<uint8_t>(PrimitiveTopology::PointList), 0);
    EXPECT_EQ(static_cast<uint8_t>(PrimitiveTopology::LineList), 1);
    EXPECT_EQ(static_cast<uint8_t>(PrimitiveTopology::LineStrip), 2);
    EXPECT_EQ(static_cast<uint8_t>(PrimitiveTopology::TriangleList), 3);
    EXPECT_EQ(static_cast<uint8_t>(PrimitiveTopology::TriangleStrip), 4);
}

TEST(Phase3_RenderPipeline, PrimitiveStateDefaults)
{
    PrimitiveState state;
    EXPECT_EQ(state.topology, PrimitiveTopology::TriangleList);
    EXPECT_EQ(state.front_face, FrontFace::CCW);
    EXPECT_EQ(state.cull_mode, CullMode::None);
}

TEST(Phase3_RenderPipeline, BlendOperationEnums)
{
    EXPECT_EQ(static_cast<uint8_t>(BlendOperation::Add), 0);
    EXPECT_EQ(static_cast<uint8_t>(BlendOperation::Subtract), 1);
    EXPECT_EQ(static_cast<uint8_t>(BlendOperation::ReverseSubtract), 2);
    EXPECT_EQ(static_cast<uint8_t>(BlendOperation::Min), 3);
    EXPECT_EQ(static_cast<uint8_t>(BlendOperation::Max), 4);
}

TEST(Phase3_RenderPipeline, BlendComponentDefaults)
{
    BlendComponent blend;
    EXPECT_EQ(blend.operation, BlendOperation::Add);
    EXPECT_EQ(blend.src_factor, BlendFactor::One);
    EXPECT_EQ(blend.dst_factor, BlendFactor::Zero);
}

TEST(Phase3_RenderPipeline, BlendStateDefaults)
{
    BlendState state;
    EXPECT_EQ(state.color.operation, BlendOperation::Add);
    EXPECT_EQ(state.alpha.operation, BlendOperation::Add);
    EXPECT_EQ(state.write_mask, ColorWriteMask::All);
}

TEST(Phase3_RenderPipeline, ComparisonFunctionEnums)
{
    EXPECT_EQ(static_cast<uint8_t>(CompareFunction::Never), 0);
    EXPECT_EQ(static_cast<uint8_t>(CompareFunction::Less), 1);
    EXPECT_EQ(static_cast<uint8_t>(CompareFunction::Equal), 2);
    EXPECT_EQ(static_cast<uint8_t>(CompareFunction::Always), 7);
}

TEST(Phase3_RenderPipeline, DepthStencilStateDefaults)
{
    DepthStencilState state;
    EXPECT_EQ(state.depth_compare, CompareFunction::Less);
    EXPECT_TRUE(state.depth_write_enabled);
    EXPECT_EQ(state.stencil_front.compare, CompareFunction::Always);
    EXPECT_EQ(state.stencil_back.compare, CompareFunction::Always);
}

TEST(Phase3_RenderPipeline, ColorTargetStateDefaults)
{
    ColorTargetState target;
    EXPECT_EQ(target.format, TextureFormat::BGRA8Unorm);
    EXPECT_FALSE(target.blend.has_value());
}

TEST(Phase3_RenderPipeline, DescriptorCreation)
{
    RenderPipelineDescriptor desc;
    desc.label                = "TestPipeline";
    desc.vertex_entry_point   = "vs_main";
    desc.fragment_entry_point = "fs_main";

    EXPECT_EQ(desc.label, "TestPipeline");
    EXPECT_EQ(desc.vertex_entry_point, "vs_main");
    EXPECT_EQ(desc.fragment_entry_point, "fs_main");
}

TEST(Phase3_RenderPipeline, DefaultConstruction)
{
    RenderPipeline pipeline;
    EXPECT_FALSE(pipeline.is_valid());
}

// ============================================================================
// Compute Pipeline Tests
// ============================================================================

TEST(Phase3_ComputePipeline, DescriptorCreation)
{
    ComputePipelineDescriptor desc;
    desc.label               = "TestComputePipeline";
    desc.compute_entry_point = "cs_main";

    EXPECT_EQ(desc.label, "TestComputePipeline");
    EXPECT_EQ(desc.compute_entry_point, "cs_main");
}

TEST(Phase3_ComputePipeline, DefaultConstruction)
{
    ComputePipeline pipeline;
    EXPECT_FALSE(pipeline.is_valid());
}

// ============================================================================
// Bind Group Tests
// ============================================================================

TEST(Phase3_BindGroup, ShaderStageFlags)
{
    EXPECT_EQ(static_cast<uint8_t>(ShaderStage::None), 0x0);
    EXPECT_EQ(static_cast<uint8_t>(ShaderStage::Vertex), 0x1);
    EXPECT_EQ(static_cast<uint8_t>(ShaderStage::Fragment), 0x2);
    EXPECT_EQ(static_cast<uint8_t>(ShaderStage::Compute), 0x4);
    EXPECT_EQ(static_cast<uint8_t>(ShaderStage::All), 0x7);
}

TEST(Phase3_BindGroup, ShaderStageFlagOr)
{
    auto combined = ShaderStage::Vertex | ShaderStage::Fragment;
    EXPECT_EQ(static_cast<uint8_t>(combined), 0x3);
}

TEST(Phase3_BindGroup, ShaderStageFlagAnd)
{
    auto all    = ShaderStage::All;
    auto vertex = ShaderStage::Vertex;
    auto result = all & vertex;
    EXPECT_EQ(result, ShaderStage::Vertex);
}

TEST(Phase3_BindGroup, BindingTypeEnums)
{
    EXPECT_EQ(static_cast<uint8_t>(BindingType::UniformBuffer), 0);
    EXPECT_EQ(static_cast<uint8_t>(BindingType::StorageBuffer), 1);
    EXPECT_EQ(static_cast<uint8_t>(BindingType::Texture), 2);
    EXPECT_EQ(static_cast<uint8_t>(BindingType::Sampler), 3);
    EXPECT_EQ(static_cast<uint8_t>(BindingType::StorageTexture), 4);
}

TEST(Phase3_BindGroup, LayoutEntryCreation)
{
    BindGroupLayoutEntry entry;
    entry.binding    = 0;
    entry.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
    entry.type       = BindingType::UniformBuffer;

    EXPECT_EQ(entry.binding, 0);
    EXPECT_EQ(static_cast<uint8_t>(entry.visibility), 0x3);
    EXPECT_EQ(entry.type, BindingType::UniformBuffer);
}

TEST(Phase3_BindGroup, LayoutDescriptorCreation)
{
    BindGroupLayoutDescriptor desc;
    desc.label = "TestLayout";
    desc.entries.push_back({ .binding    = 0,
                             .visibility = ShaderStage::Vertex,
                             .type       = BindingType::UniformBuffer });

    EXPECT_EQ(desc.label, "TestLayout");
    EXPECT_EQ(desc.entries.size(), 1);
    EXPECT_EQ(desc.entries[0].binding, 0);
}

TEST(Phase3_BindGroup, LayoutDefaultConstruction)
{
    BindGroupLayout layout;
    EXPECT_FALSE(layout.is_valid());
}

TEST(Phase3_BindGroup, EntryDefaults)
{
    BindGroupEntry entry;
    entry.binding = 0;

    EXPECT_EQ(entry.binding, 0);
    EXPECT_EQ(entry.offset, 0);
    EXPECT_EQ(entry.size, UINT64_MAX);
}

TEST(Phase3_BindGroup, DescriptorCreation)
{
    BindGroupDescriptor desc;
    desc.label = "TestBindGroup";

    EXPECT_EQ(desc.label, "TestBindGroup");
    EXPECT_TRUE(desc.entries.empty());
}

TEST(Phase3_BindGroup, DefaultConstruction)
{
    BindGroup bind_group;
    EXPECT_FALSE(bind_group.is_valid());
}

// ============================================================================
// Documentation Tests (Skipped - No Device)
// ============================================================================

TEST(Phase3_Documentation, DISABLED_FullPipelineWorkflow)
{
    // This test demonstrates the complete Phase 3 workflow:
    //
    // 1. Create shader module
    // ShaderModuleDescriptor shader_desc;
    // shader_desc.code = "...WGSL code...";
    // auto shader = device.create_shader_module(shader_desc);
    //
    // 2. Create bind group layout
    // BindGroupLayoutDescriptor layout_desc;
    // layout_desc.entries.push_back({
    //     .binding = 0,
    //     .visibility = ShaderStage::Vertex,
    //     .type = BindingType::UniformBuffer
    // });
    // auto layout = device.create_bind_group_layout(layout_desc);
    //
    // 3. Create render pipeline
    // RenderPipelineDescriptor pipeline_desc;
    // pipeline_desc.vertex_shader = shader;
    // pipeline_desc.fragment_shader = shader;
    // pipeline_desc.primitive.topology = PrimitiveTopology::TriangleList;
    // auto pipeline = device.create_render_pipeline(pipeline_desc);
    //
    // 4. Create bind group
    // BindGroupDescriptor bind_desc;
    // bind_desc.layout = layout;
    // bind_desc.entries.push_back({.binding = 0, .buffer = uniform_buffer});
    // auto bind_group = device.create_bind_group(bind_desc);
    //
    // 5. Use in render pass
    // auto pass = encoder.begin_render_pass(pass_desc);
    // pass.set_pipeline(pipeline);
    // pass.set_bind_group(0, bind_group);
    // pass.set_viewport(0, 0, 800, 600, 0, 1);
    // pass.set_scissor_rect(0, 0, 800, 600);
    // pass.draw(3, 1, 0, 0);
    // pass.end();
}
