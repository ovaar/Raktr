/*!
 * @file test_command_recording.cpp
 * @brief Test command recording API (Phase 2).
 */

#include "command_encoder.h"
#include "compute_pass_encoder.h"
#include "queue.h"
#include "render_context.h"
#include "render_pass_encoder.h"
#include "render_pipeline.h"
#include "shader_module.h"
#include "window/window.h"

#include <gtest/gtest.h>

using namespace raktr::render;

/*!
 * @brief Test demonstrates command recording API structure.
 *
 * This test shows the intended usage pattern but doesn't execute
 * because we need a real device instance. It serves as documentation
 * for how the command recording API should be used.
 */
TEST(CommandRecording, API_Structure)
{
    // This test demonstrates the API structure but is skipped
    // because we need a real WgpuDevice instance to test execution.
    GTEST_SKIP() << "Skipping - requires real device instance for full test";

    // Example usage (commented out - needs real device):
    /*
    auto device = create_device();

    // Create command encoder
    auto encoder = device.create_command_encoder("Example Commands");

    // Begin render pass
    RenderPassDescriptor render_desc;
    render_desc.color_attachments[0].view = surface_view;
    render_desc.color_attachments[0].load_op = LoadOp::Clear;
    render_desc.color_attachments[0].clear_color = {0.0, 0.0, 0.0, 1.0};
    render_desc.color_attachment_count = 1;

    auto render_pass = encoder.begin_render_pass(render_desc);
    render_pass.set_vertex_buffer(0, vertex_buffer);
    render_pass.set_index_buffer(index_buffer);
    render_pass.draw_indexed(36, 1, 0, 0, 0);
    render_pass.end();

    // Begin compute pass
    auto compute_pass = encoder.begin_compute_pass({});
    compute_pass.dispatch(16, 16, 1);
    compute_pass.end();

    // Copy between buffers
    encoder.copy_buffer_to_buffer(staging_buffer, 0, gpu_buffer, 0, 1024);

    // Finish and submit
    CommandBuffer commands = encoder.finish();
    device.queue().submit({commands});
    */
}

/*!
 * @brief Test RenderPassDescriptor structure.
 */
TEST(CommandRecording, RenderPassDescriptor_Creation)
{
    RenderPassDescriptor desc;

    // Verify default values
    EXPECT_EQ(desc.color_attachment_count, 0);
    EXPECT_FALSE(desc.depth_stencil_attachment.has_value());

    // Set up color attachment
    desc.color_attachments[0].view        = reinterpret_cast<void*>(0x1234);
    desc.color_attachments[0].load_op     = LoadOp::Clear;
    desc.color_attachments[0].store_op    = StoreOp::Store;
    desc.color_attachments[0].clear_color = { 0.1, 0.2, 0.3, 1.0 };
    desc.color_attachment_count           = 1;

    EXPECT_EQ(desc.color_attachment_count, 1);
    EXPECT_EQ(desc.color_attachments[0].view, reinterpret_cast<void*>(0x1234));
    EXPECT_EQ(desc.color_attachments[0].load_op, LoadOp::Clear);
    EXPECT_EQ(desc.color_attachments[0].store_op, StoreOp::Store);
    EXPECT_DOUBLE_EQ(desc.color_attachments[0].clear_color.r, 0.1);

    // Set up depth attachment
    RenderPassDepthStencilAttachment depth_att;
    depth_att.view                = reinterpret_cast<void*>(0x5678);
    depth_att.depth_load_op       = LoadOp::Clear;
    depth_att.depth_clear_value   = 1.0f;
    depth_att.depth_read_only     = false;
    desc.depth_stencil_attachment = depth_att;

    EXPECT_TRUE(desc.depth_stencil_attachment.has_value());
    EXPECT_EQ(desc.depth_stencil_attachment->view, reinterpret_cast<void*>(0x5678));
    EXPECT_FLOAT_EQ(desc.depth_stencil_attachment->depth_clear_value, 1.0f);
}

/*!
 * @brief Test ComputePassDescriptor structure.
 */
TEST(CommandRecording, ComputePassDescriptor_Creation)
{
    ComputePassDescriptor desc;

    // Currently no fields in descriptor
    // This test ensures the type compiles and can be instantiated
    (void)desc;

    SUCCEED();
}

/*!
 * @brief Test LoadOp enum values.
 */
TEST(CommandRecording, LoadOp_Values)
{
    LoadOp load  = LoadOp::Load;
    LoadOp clear = LoadOp::Clear;

    EXPECT_NE(load, clear);
}

/*!
 * @brief Test StoreOp enum values.
 */
TEST(CommandRecording, StoreOp_Values)
{
    StoreOp store   = StoreOp::Store;
    StoreOp discard = StoreOp::Discard;

    EXPECT_NE(store, discard);
}

/*!
 * @brief Test ClearColor structure.
 */
TEST(CommandRecording, ClearColor_Creation)
{
    ClearColor black = { 0.0, 0.0, 0.0, 1.0 };
    EXPECT_DOUBLE_EQ(black.r, 0.0);
    EXPECT_DOUBLE_EQ(black.g, 0.0);
    EXPECT_DOUBLE_EQ(black.b, 0.0);
    EXPECT_DOUBLE_EQ(black.a, 1.0);

    ClearColor red = { 1.0, 0.0, 0.0, 1.0 };
    EXPECT_DOUBLE_EQ(red.r, 1.0);
    EXPECT_DOUBLE_EQ(red.g, 0.0);
    EXPECT_DOUBLE_EQ(red.b, 0.0);
    EXPECT_DOUBLE_EQ(red.a, 1.0);

    ClearColor transparent = { 0.0, 0.0, 0.0, 0.0 };
    EXPECT_DOUBLE_EQ(transparent.a, 0.0);
}

/*!
 * @brief Test RenderPassEncoder set_pipeline() method.
 *
 * This test verifies that the set_pipeline() method is available
 * on the RenderPassEncoder API.
 */
TEST(CommandRecording, RenderPassEncoder_SetPipeline)
{
    WindowConfig window_config;
    window_config.width     = 800;
    window_config.height    = 600;
    window_config.title     = "RenderPassEncoder SetPipeline Test";
    window_config.resizable = true;

    RenderConfig render_config;
    render_config.backend           = BackendType::WebGPU;
    render_config.enable_validation = false;

    auto window_result = create_window(window_config);
    ASSERT_TRUE(window_result.has_value()) << "Failed to create window";
    auto& window = window_result.value();

    auto render_context = create_render_context();
    ASSERT_NE(render_context, nullptr);

    auto render_ctx_result = render_context->initialize(render_config, window.get());
    ASSERT_TRUE(render_ctx_result.has_value()) << "Failed to initialize RenderContext";

    auto device = render_context->device();
    ASSERT_NE(device, nullptr);

    // Create cube geometry
    float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f
    };
    auto vertex_buffer = device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vertex_buffer.has_value());

    uint32_t indices[] = {
        0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 4, 7, 3, 4, 3, 0, 1, 2, 6, 1, 6, 5, 0, 1, 5, 0, 5, 4, 3, 6, 2, 3, 7, 6
    };
    auto index_buffer = device->create_index_buffer(std::as_bytes(std::span(indices)));
    ASSERT_TRUE(index_buffer.has_value());

    // Create shader module
    const char* shader_code = R"(
        @vertex
        fn vs_main(@location(0) position: vec3<f32>) -> @builtin(position) vec4<f32> {
            return vec4<f32>(position, 1.0);
        }

        @fragment
        fn fs_main() -> @location(0) vec4<f32> {
            return vec4<f32>(1.0, 0.0, 0.0, 1.0);
        }
    )";

    ShaderModuleDescriptor shader_desc;
    shader_desc.label  = "Test Shader";
    shader_desc.code   = shader_code;
    auto shader_result = device->create_shader_module(shader_desc);
    ASSERT_TRUE(shader_result.has_value()) << "Failed to create shader module";

    // Create pipeline
    RenderPipelineDescriptor pipeline_desc;
    pipeline_desc.label              = "Test Pipeline";
    pipeline_desc.vertex_shader      = std::move(shader_result.value());
    pipeline_desc.vertex_entry_point = "vs_main";

    // Create another shader module for fragment stage
    auto fragment_shader_result = device->create_shader_module(shader_desc);
    ASSERT_TRUE(fragment_shader_result.has_value()) << "Failed to create fragment shader module";
    pipeline_desc.fragment_shader      = std::move(fragment_shader_result.value());
    pipeline_desc.fragment_entry_point = "fs_main";

    // Configure vertex layout
    VertexAttribute position_attrib;
    position_attrib.format          = VertexFormat::Float32x3;
    position_attrib.offset          = 0;
    position_attrib.shader_location = 0;

    VertexBufferLayout vertex_layout;
    vertex_layout.array_stride = sizeof(float) * 3;
    vertex_layout.step_mode    = VertexStepMode::Vertex;
    vertex_layout.attributes   = { position_attrib };

    pipeline_desc.vertex_buffers = { vertex_layout };

    // Configure color target
    ColorTargetState color_target;
    color_target.format         = TextureFormat::BGRA8Unorm;
    pipeline_desc.color_targets = { color_target };

    // Configure primitive state
    pipeline_desc.primitive.topology = PrimitiveTopology::TriangleList;

    auto pipeline_result = device->create_render_pipeline(pipeline_desc);
    ASSERT_TRUE(pipeline_result.has_value()) << "Failed to create render pipeline";
    RenderPipeline pipeline = std::move(pipeline_result.value());

    // Create encoder
    auto encoder_result = device->create_command_encoder("Test");
    ASSERT_TRUE(encoder_result.has_value()) << "Failed to create command encoder";
    CommandEncoder encoder = std::move(encoder_result.value());

    // Begin render pass
    RenderPassDescriptor render_desc;
    render_desc.color_attachments[0].view    = device->get_surface_view();
    render_desc.color_attachments[0].load_op = LoadOp::Clear;
    render_desc.color_attachment_count       = 1;

    RenderPassEncoder pass = encoder.begin_render_pass(render_desc);

    // Set pipeline
    pass.set_pipeline(pipeline);

    // Set buffers and draw
    pass.set_vertex_buffer(0, vertex_buffer.value(), 0, sizeof(vertices));
    pass.set_index_buffer(index_buffer.value(), 0, sizeof(indices));
    pass.draw(3, 1, 0, 0);

    pass.end();

    CommandBuffer                commands = encoder.finish();
    std::array<CommandBuffer, 1> command_list{ commands };
    auto queue_result = device->queue();
    ASSERT_TRUE(queue_result.has_value()) << "Failed to get queue";
    queue_result.value().submit(command_list);
}

/*!
 * @brief Document the command recording workflow.
 *
 * This test describes the complete command recording workflow:
 * 1. Create CommandEncoder from device
 * 2. Begin render/compute passes
 * 3. Record commands into passes
 * 4. End passes
 * 5. Finish encoder to create CommandBuffer
 * 6. Submit CommandBuffer to Queue
 */
TEST(CommandRecording, Workflow_Documentation)
{
    GTEST_SKIP() << "Documentation test - shows workflow pattern";

    /*
    Complete command recording workflow:

    // 1. Create encoder
    CommandEncoder encoder = device.create_command_encoder("Frame Commands");

    // 2. Begin render pass
    RenderPassDescriptor render_desc;
    render_desc.color_attachments[0].view = get_surface_view();
    render_desc.color_attachments[0].load_op = LoadOp::Clear;
    render_desc.color_attachments[0].clear_color = {0.0, 0.0, 0.0, 1.0};
    render_desc.color_attachment_count = 1;

    render_desc.depth_stencil_attachment = RenderPassDepthStencilAttachment{};
    render_desc.depth_stencil_attachment->view = get_depth_view();
    render_desc.depth_stencil_attachment->depth_load_op = LoadOp::Clear;
    render_desc.depth_stencil_attachment->depth_clear_value = 1.0f;

    // 3. Record graphics commands
    {
        RenderPassEncoder pass = encoder.begin_render_pass(render_desc);

        // Set pipeline
        pass.set_pipeline(render_pipeline);

        // Set buffers
        pass.set_vertex_buffer(0, vertex_buffer, 0, vertex_size);
        pass.set_index_buffer(index_buffer, 0, index_size);

        // Draw
        pass.draw_indexed(index_count, instance_count, 0, 0, 0);

        // Must end pass before encoder.finish()
        pass.end();
    }

    // 4. Begin compute pass (optional)
    {
        ComputePassEncoder pass = encoder.begin_compute_pass({});

        // Dispatch compute work
        pass.dispatch(workgroup_x, workgroup_y, workgroup_z);

        pass.end();
    }

    // 5. Record copies
    encoder.copy_buffer_to_buffer(src, 0, dst, 0, copy_size);

    // 6. Finish recording
    CommandBuffer commands = encoder.finish();

    // 7. Submit to queue
    Queue queue = device.queue();
    queue.submit({commands});

    // 8. Present (if rendering to swapchain)
    device.present();
    */
}
