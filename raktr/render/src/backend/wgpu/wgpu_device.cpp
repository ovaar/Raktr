/*!
 * @file wgpu_device.cpp
 * @brief WebGPU device implementation.
 */

#include "wgpu_device.h"
#include "window/window.h"
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace raktr::render::backend
{

    // Helper to create WGPUStringView from C string
    static WGPUStringView make_string_view(const char* str)
    {
        WGPUStringView view = {};
        view.data           = str;
        view.length         = str ? strlen(str) : 0;
        return view;
    }

    std::expected<std::unique_ptr<WgpuDevice>, std::error_code>
    WgpuDevice::create(Window* window, bool enable_validation)
    {
        std::unique_ptr<WgpuDevice> device(new WgpuDevice());

        auto result = device->initialize(window, enable_validation);
        if (!result)
        {
            return std::unexpected(result.error());
        }

        return device;
    }

    WgpuDevice::~WgpuDevice()
    {
        cleanup();
    }

    std::expected<void, std::error_code>
    WgpuDevice::initialize(Window* window, bool /* enable_validation */)
    {
        if (!window)
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Create instance
        WGPUInstanceDescriptor instance_desc = {};
        instance_desc.nextInChain            = nullptr;
        _instance                            = wgpuCreateInstance(&instance_desc);
        if (!_instance)
        {
            spdlog::error("Failed to create WebGPU instance");
            return std::unexpected(make_error_code(RenderError::InitializationFailed));
        }

        // Create surface from window
        _swapchain_width  = window->width();
        _swapchain_height = window->height();

        // Initialize viewport with default aspect ratio (16:9)
        _viewport = calculate_viewport(_swapchain_width, _swapchain_height, _aspect_ratio, _custom_aspect_ratio);

#ifdef _WIN32
        WGPUSurfaceSourceWindowsHWND surface_source = {};
        surface_source.chain.sType                  = WGPUSType_SurfaceSourceWindowsHWND;
        surface_source.chain.next                   = nullptr;
        surface_source.hinstance                    = GetModuleHandle(nullptr);
        surface_source.hwnd                         = static_cast<HWND>(window->native_handle());

        WGPUSurfaceDescriptor surface_desc = {};
        surface_desc.nextInChain           = reinterpret_cast<WGPUChainedStruct*>(&surface_source);
        surface_desc.label                 = make_string_view("Main Window Surface");

        _surface = wgpuInstanceCreateSurface(_instance, &surface_desc);
#else
        // TODO: Add Linux/macOS surface creation
        spdlog::error("Surface creation not implemented for this platform");
        return std::unexpected(make_error_code(RenderError::InitializationFailed));
#endif

        if (!_surface)
        {
            spdlog::error("Failed to create WebGPU surface");
            return std::unexpected(make_error_code(RenderError::InitializationFailed));
        }

        // Request adapter
        WGPURequestAdapterOptions adapter_opts = {};
        adapter_opts.nextInChain               = nullptr;
        adapter_opts.compatibleSurface         = _surface;
        adapter_opts.powerPreference           = WGPUPowerPreference_HighPerformance;

        WGPURequestAdapterCallbackInfo adapter_callback = {};
        adapter_callback.mode                           = WGPUCallbackMode_WaitAnyOnly;
        adapter_callback.callback                       = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* /* userdata2 */)
        {
            if (status == WGPURequestAdapterStatus_Success && adapter)
            {
                *static_cast<WGPUAdapter*>(userdata1) = adapter;
            }
            else
            {
                const std::string_view msg_view(message.data, message.length);
                spdlog::error("Failed to request adapter: {}", msg_view);
            }
        };
        adapter_callback.userdata1 = &_adapter;
        adapter_callback.userdata2 = nullptr;

        wgpuInstanceRequestAdapter(_instance, &adapter_opts, adapter_callback);
        wgpuInstanceProcessEvents(_instance);

        if (!_adapter)
        {
            spdlog::error("Failed to obtain WebGPU adapter");
            return std::unexpected(make_error_code(RenderError::InitializationFailed));
        }

        // Request device
        WGPUDeviceDescriptor device_desc = {};
        device_desc.nextInChain          = nullptr;
        device_desc.label                = make_string_view("Primary Device");
        device_desc.requiredFeatureCount = 0;
        device_desc.requiredFeatures     = nullptr;
        device_desc.requiredLimits       = nullptr;
        device_desc.defaultQueue.label   = make_string_view("Default Queue");

        WGPURequestDeviceCallbackInfo device_callback = {};
        device_callback.mode                          = WGPUCallbackMode_WaitAnyOnly;
        device_callback.callback                      = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* /* userdata2 */)
        {
            if (status == WGPURequestDeviceStatus_Success && device)
            {
                *static_cast<WGPUDevice*>(userdata1) = device;
            }
            else
            {
                const std::string_view msg_view(message.data, message.length);
                spdlog::error("Failed to request device: {}", msg_view);
            }
        };
        device_callback.userdata1 = &_device;
        device_callback.userdata2 = nullptr;

        wgpuAdapterRequestDevice(_adapter, &device_desc, device_callback);
        wgpuInstanceProcessEvents(_instance);

        if (!_device)
        {
            spdlog::error("Failed to obtain WebGPU device");
            return std::unexpected(make_error_code(RenderError::DeviceCreationFailed));
        }

        // Get queue
        _queue = wgpuDeviceGetQueue(_device);
        if (!_queue)
        {
            spdlog::error("Failed to obtain WebGPU queue");
            return std::unexpected(make_error_code(RenderError::DeviceCreationFailed));
        }

        // Configure surface (no swapchain in new API, configure surface directly)
        WGPUSurfaceConfiguration surface_config = {};
        surface_config.nextInChain              = nullptr;
        surface_config.device                   = _device;
        surface_config.format                   = _swapchain_format;
        surface_config.usage                    = WGPUTextureUsage_RenderAttachment;
        surface_config.width                    = _swapchain_width;
        surface_config.height                   = _swapchain_height;
        surface_config.presentMode              = WGPUPresentMode_Fifo; // VSync
        surface_config.alphaMode                = WGPUCompositeAlphaMode_Auto;

        wgpuSurfaceConfigure(_surface, &surface_config);

        // Load and compile shader
        const char* wgsl_source = R"(
// Uniform buffer for transformation matrix
struct Uniforms {
    mvp: mat4x4<f32>,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

// Vertex shader
struct VertexInput {
    @location(0) position: vec3<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
};

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    output.position = uniforms.mvp * vec4<f32>(input.position, 1.0);
    output.color = input.position * 0.5 + 0.5;
    return output;
}

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(input.color, 1.0);
}
)";

        auto shader_result = create_shader_module(wgsl_source, "Basic Shader");
        if (!shader_result)
        {
            spdlog::error("Failed to create shader module");
            return std::unexpected(shader_result.error());
        }

        // Create render pipeline
        auto pipeline_result = create_render_pipeline();
        if (!pipeline_result)
        {
            spdlog::error("Failed to create render pipeline");
            return std::unexpected(pipeline_result.error());
        }

        // Create default uniform buffer with identity matrix for backward compatibility
        // This allows existing tests to work without providing a uniform buffer
        float identity_matrix[16] = {
            1.0f, 0.0f, 0.0f, 0.0f, // Column 0
            0.0f,
            1.0f,
            0.0f,
            0.0f, // Column 1
            0.0f,
            0.0f,
            1.0f,
            0.0f, // Column 2
            0.0f,
            0.0f,
            0.0f,
            1.0f // Column 3
        };
        auto uniform_result = create_uniform_buffer(sizeof(identity_matrix));
        if (!uniform_result)
        {
            spdlog::error("Failed to create default uniform buffer");
            return std::unexpected(uniform_result.error());
        }
        _default_uniform_buffer = uniform_result.value();

        // Upload identity matrix
        auto identity_data = std::as_bytes(std::span(identity_matrix));
        auto update_result = update_uniform_buffer(_default_uniform_buffer, identity_data);
        if (!update_result)
        {
            spdlog::error("Failed to update default uniform buffer");
            return std::unexpected(update_result.error());
        }

        // Set as current uniform buffer
        set_uniform_buffer(_default_uniform_buffer);

        spdlog::info("WebGPU device initialized successfully ({}x{})", _swapchain_width, _swapchain_height);
        return {};
    }

    std::expected<void, std::error_code>
    WgpuDevice::create_shader_module(const char* wgsl_source, const char* label)
    {
        if (!_device || !wgsl_source)
        {
            return std::unexpected(make_error_code(RenderError::ShaderCompilationFailed));
        }

        WGPUShaderSourceWGSL wgsl_desc = {};
        wgsl_desc.chain.sType          = WGPUSType_ShaderSourceWGSL;
        wgsl_desc.chain.next           = nullptr;
        wgsl_desc.code                 = make_string_view(wgsl_source);

        WGPUShaderModuleDescriptor shader_desc = {};
        shader_desc.nextInChain                = reinterpret_cast<WGPUChainedStruct*>(&wgsl_desc);
        shader_desc.label                      = make_string_view(label);

        _shader_module = wgpuDeviceCreateShaderModule(_device, &shader_desc);
        if (!_shader_module)
        {
            spdlog::error("Failed to create shader module: {}", label);
            return std::unexpected(make_error_code(RenderError::ShaderCompilationFailed));
        }

        spdlog::info("Shader module '{}' created successfully", label);
        return {};
    }

    std::expected<void, std::error_code>
    WgpuDevice::create_render_pipeline()
    {
        if (!_device || !_shader_module)
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Create bind group layout for uniform buffer
        WGPUBindGroupLayoutEntry bind_group_layout_entry = {};
        bind_group_layout_entry.binding                  = 0;
        bind_group_layout_entry.visibility               = WGPUShaderStage_Vertex;
        bind_group_layout_entry.buffer.type              = WGPUBufferBindingType_Uniform;
        bind_group_layout_entry.buffer.hasDynamicOffset  = false;
        bind_group_layout_entry.buffer.minBindingSize    = 0;

        WGPUBindGroupLayoutDescriptor bind_group_layout_desc = {};
        bind_group_layout_desc.entryCount                    = 1;
        bind_group_layout_desc.entries                       = &bind_group_layout_entry;

        _bind_group_layout = wgpuDeviceCreateBindGroupLayout(_device, &bind_group_layout_desc);
        if (!_bind_group_layout)
        {
            spdlog::error("Failed to create bind group layout");
            return std::unexpected(make_error_code(RenderError::InitializationFailed));
        }

        // Pipeline layout with bind group
        WGPUPipelineLayoutDescriptor pipeline_layout_desc = {};
        pipeline_layout_desc.bindGroupLayoutCount         = 1;
        pipeline_layout_desc.bindGroupLayouts             = &_bind_group_layout;

        WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(_device, &pipeline_layout_desc);
        if (!pipeline_layout)
        {
            wgpuBindGroupLayoutRelease(_bind_group_layout);
            _bind_group_layout = nullptr;
            spdlog::error("Failed to create pipeline layout");
            return std::unexpected(make_error_code(RenderError::InitializationFailed));
        }

        // Vertex buffer layout
        WGPUVertexAttribute vertex_attribute = {};
        vertex_attribute.format              = WGPUVertexFormat_Float32x3;
        vertex_attribute.offset              = 0;
        vertex_attribute.shaderLocation      = 0;

        WGPUVertexBufferLayout vertex_buffer_layout = {};
        vertex_buffer_layout.arrayStride            = 3 * sizeof(float);
        vertex_buffer_layout.stepMode               = WGPUVertexStepMode_Vertex;
        vertex_buffer_layout.attributeCount         = 1;
        vertex_buffer_layout.attributes             = &vertex_attribute;

        // Color target state
        WGPUColorTargetState color_target = {};
        color_target.format               = _swapchain_format;
        color_target.writeMask            = WGPUColorWriteMask_All;
        color_target.blend                = nullptr; // No blending

        // Fragment state
        WGPUFragmentState fragment_state = {};
        fragment_state.module            = _shader_module;
        fragment_state.entryPoint        = make_string_view("fs_main");
        fragment_state.targetCount       = 1;
        fragment_state.targets           = &color_target;
        fragment_state.constantCount     = 0;
        fragment_state.constants         = nullptr;

        // Pipeline descriptor
        WGPURenderPipelineDescriptor pipeline_desc = {};
        pipeline_desc.nextInChain                  = nullptr;
        pipeline_desc.label                        = make_string_view("Basic Render Pipeline");
        pipeline_desc.layout                       = pipeline_layout;

        // Vertex state
        pipeline_desc.vertex.module        = _shader_module;
        pipeline_desc.vertex.entryPoint    = make_string_view("vs_main");
        pipeline_desc.vertex.bufferCount   = 1;
        pipeline_desc.vertex.buffers       = &vertex_buffer_layout;
        pipeline_desc.vertex.constantCount = 0;
        pipeline_desc.vertex.constants     = nullptr;

        // Primitive state
        pipeline_desc.primitive.topology         = WGPUPrimitiveTopology_TriangleList;
        pipeline_desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipeline_desc.primitive.frontFace        = WGPUFrontFace_CCW;
        pipeline_desc.primitive.cullMode         = WGPUCullMode_None;

        // Multisample state
        pipeline_desc.multisample.count                  = 1;
        pipeline_desc.multisample.mask                   = 0xFFFFFFFF;
        pipeline_desc.multisample.alphaToCoverageEnabled = false;

        // Fragment state
        pipeline_desc.fragment = &fragment_state;

        // No depth/stencil for now
        pipeline_desc.depthStencil = nullptr;

        _render_pipeline = wgpuDeviceCreateRenderPipeline(_device, &pipeline_desc);

        // Release pipeline layout (retained by pipeline)
        wgpuPipelineLayoutRelease(pipeline_layout);

        if (!_render_pipeline)
        {
            spdlog::error("Failed to create render pipeline");
            return std::unexpected(make_error_code(RenderError::InitializationFailed));
        }

        spdlog::info("Render pipeline created successfully");
        return {};
    }

    void WgpuDevice::cleanup()
    {
        // Release buffers
        for (auto buffer : _buffers)
        {
            if (buffer)
            {
                wgpuBufferRelease(buffer);
            }
        }
        _buffers.clear();

        // Release shader and pipeline resources
        if (_render_pipeline)
        {
            wgpuRenderPipelineRelease(_render_pipeline);
            _render_pipeline = nullptr;
        }
        if (_shader_module)
        {
            wgpuShaderModuleRelease(_shader_module);
            _shader_module = nullptr;
        }

        // Release bind group resources
        if (_current_bind_group)
        {
            wgpuBindGroupRelease(_current_bind_group);
            _current_bind_group = nullptr;
        }
        if (_bind_group_layout)
        {
            wgpuBindGroupLayoutRelease(_bind_group_layout);
            _bind_group_layout = nullptr;
        }

        // Release any pending surface texture
        if (_current_surface_texture)
        {
            wgpuTextureRelease(_current_surface_texture);
            _current_surface_texture = nullptr;
        }

        // Release WebGPU resources (no swapchain in new API)
        if (_queue)
        {
            wgpuQueueRelease(_queue);
            _queue = nullptr;
        }
        if (_device)
        {
            wgpuDeviceRelease(_device);
            _device = nullptr;
        }
        if (_surface)
        {
            wgpuSurfaceRelease(_surface);
            _surface = nullptr;
        }
        if (_adapter)
        {
            wgpuAdapterRelease(_adapter);
            _adapter = nullptr;
        }
        if (_instance)
        {
            wgpuInstanceRelease(_instance);
            _instance = nullptr;
        }
    }

    std::expected<Buffer, std::error_code>
    WgpuDevice::create_vertex_buffer(std::span<const std::byte> data)
    {
        if (data.empty() || !_device)
        {
            return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
        }

        WGPUBufferDescriptor buffer_desc = {};
        buffer_desc.nextInChain          = nullptr;
        buffer_desc.label                = make_string_view("Vertex Buffer");
        buffer_desc.usage                = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        buffer_desc.size                 = data.size();
        buffer_desc.mappedAtCreation     = false;

        WGPUBuffer wgpu_buffer = wgpuDeviceCreateBuffer(_device, &buffer_desc);
        if (!wgpu_buffer)
        {
            return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
        }

        // Upload data
        wgpuQueueWriteBuffer(_queue, wgpu_buffer, 0, data.data(), data.size());

        // Store buffer for cleanup
        _buffers.push_back(wgpu_buffer);

        // Return handle (use buffer index as ID)
        return Buffer(static_cast<uint64_t>(_buffers.size() - 1), BufferType::Vertex);
    }

    std::expected<Buffer, std::error_code>
    WgpuDevice::create_index_buffer(std::span<const std::byte> data)
    {
        if (data.empty() || !_device)
        {
            return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
        }

        WGPUBufferDescriptor buffer_desc = {};
        buffer_desc.nextInChain          = nullptr;
        buffer_desc.label                = make_string_view("Index Buffer");
        buffer_desc.usage                = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        buffer_desc.size                 = data.size();
        buffer_desc.mappedAtCreation     = false;

        WGPUBuffer wgpu_buffer = wgpuDeviceCreateBuffer(_device, &buffer_desc);
        if (!wgpu_buffer)
        {
            return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
        }

        // Upload data
        wgpuQueueWriteBuffer(_queue, wgpu_buffer, 0, data.data(), data.size());

        // Store buffer for cleanup
        _buffers.push_back(wgpu_buffer);

        // Return handle (use buffer index as ID)
        return Buffer(static_cast<uint64_t>(_buffers.size() - 1), BufferType::Index);
    }

    std::expected<Buffer, std::error_code>
    WgpuDevice::create_uniform_buffer(size_t size)
    {
        if (size == 0 || !_device)
        {
            return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
        }

        // WebGPU requires uniform buffer size to be multiple of 16 bytes
        size_t aligned_size = (size + 15) & ~15;

        WGPUBufferDescriptor buffer_desc = {};
        buffer_desc.nextInChain          = nullptr;
        buffer_desc.label                = make_string_view("Uniform Buffer");
        buffer_desc.usage                = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        buffer_desc.size                 = aligned_size;
        buffer_desc.mappedAtCreation     = false;

        WGPUBuffer wgpu_buffer = wgpuDeviceCreateBuffer(_device, &buffer_desc);
        if (!wgpu_buffer)
        {
            return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
        }

        // Store buffer for cleanup
        _buffers.push_back(wgpu_buffer);

        // Return handle (use buffer index as ID)
        return Buffer(static_cast<uint64_t>(_buffers.size() - 1), BufferType::Uniform);
    }

    std::expected<void, std::error_code>
    WgpuDevice::update_uniform_buffer(const Buffer& buffer, std::span<const std::byte> data)
    {
        if (data.empty() || !_queue)
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Validate buffer ID
        if (buffer.id() >= _buffers.size())
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        WGPUBuffer wgpu_buffer = _buffers[buffer.id()];
        if (!wgpu_buffer)
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Upload data to uniform buffer
        wgpuQueueWriteBuffer(_queue, wgpu_buffer, 0, data.data(), data.size());

        return {};
    }

    void WgpuDevice::set_uniform_buffer(const Buffer& buffer)
    {
        if (!_device || !_bind_group_layout || buffer.id() >= _buffers.size())
        {
            return;
        }

        WGPUBuffer wgpu_buffer = _buffers[buffer.id()];
        if (!wgpu_buffer)
        {
            return;
        }

        // Create bind group entry for the uniform buffer
        WGPUBindGroupEntry bind_group_entry = {};
        bind_group_entry.binding            = 0;
        bind_group_entry.buffer             = wgpu_buffer;
        bind_group_entry.offset             = 0;
        bind_group_entry.size               = WGPU_WHOLE_SIZE;

        // Create bind group
        WGPUBindGroupDescriptor bind_group_desc = {};
        bind_group_desc.nextInChain             = nullptr;
        bind_group_desc.label                   = make_string_view("Uniform Bind Group");
        bind_group_desc.layout                  = _bind_group_layout;
        bind_group_desc.entryCount              = 1;
        bind_group_desc.entries                 = &bind_group_entry;

        // Release old bind group if exists
        if (_current_bind_group)
        {
            wgpuBindGroupRelease(_current_bind_group);
        }

        _current_bind_group = wgpuDeviceCreateBindGroup(_device, &bind_group_desc);
    }

    std::expected<void, std::error_code>
    WgpuDevice::draw_indexed(const Buffer& vertex_buffer,
                             const Buffer& index_buffer,
                             uint32_t      index_count)
    {
        if (!_surface || !_queue || !_render_pipeline)
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Validate buffer IDs
        if (vertex_buffer.id() >= _buffers.size() || index_buffer.id() >= _buffers.size())
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        WGPUBuffer wgpu_vertex_buffer = _buffers[vertex_buffer.id()];
        WGPUBuffer wgpu_index_buffer  = _buffers[index_buffer.id()];

        if (!wgpu_vertex_buffer || !wgpu_index_buffer)
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Get current texture from surface
        WGPUSurfaceTexture surface_texture;
        wgpuSurfaceGetCurrentTexture(_surface, &surface_texture);

        if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
            surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
        {
            spdlog::error("Failed to get surface texture for drawing: {}", static_cast<int>(surface_texture.status));
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Create texture view
        WGPUTextureViewDescriptor view_desc = {};
        view_desc.nextInChain               = nullptr;
        view_desc.label                     = make_string_view("Surface Texture View");
        view_desc.format                    = _swapchain_format;
        view_desc.dimension                 = WGPUTextureViewDimension_2D;
        view_desc.baseMipLevel              = 0;
        view_desc.mipLevelCount             = 1;
        view_desc.baseArrayLayer            = 0;
        view_desc.arrayLayerCount           = 1;
        view_desc.aspect                    = WGPUTextureAspect_All;

        WGPUTextureView backbuffer_view = wgpuTextureCreateView(surface_texture.texture, &view_desc);

        // Create command encoder
        WGPUCommandEncoderDescriptor encoder_desc = {};
        encoder_desc.nextInChain                  = nullptr;
        encoder_desc.label                        = make_string_view("Draw Command Encoder");
        WGPUCommandEncoder encoder                = wgpuDeviceCreateCommandEncoder(_device, &encoder_desc);

        // Create render pass
        WGPURenderPassColorAttachment color_attachment = {};
        color_attachment.view                          = backbuffer_view;
        color_attachment.depthSlice                    = WGPU_DEPTH_SLICE_UNDEFINED;
        color_attachment.resolveTarget                 = nullptr;
        color_attachment.loadOp                        = WGPULoadOp_Clear;
        color_attachment.storeOp                       = WGPUStoreOp_Store;
        color_attachment.clearValue                    = { 0.1, 0.2, 0.3, 1.0 }; // Clear to dark blue-gray

        WGPURenderPassDescriptor render_pass_desc = {};
        render_pass_desc.nextInChain              = nullptr;
        render_pass_desc.label                    = make_string_view("Draw Render Pass");
        render_pass_desc.colorAttachmentCount     = 1;
        render_pass_desc.colorAttachments         = &color_attachment;
        render_pass_desc.depthStencilAttachment   = nullptr;

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);

        // Set pipeline and buffers
        wgpuRenderPassEncoderSetPipeline(pass, _render_pipeline);

        // Set viewport to maintain aspect ratio
        wgpuRenderPassEncoderSetViewport(pass,
                                         static_cast<float>(_viewport.x),
                                         static_cast<float>(_viewport.y),
                                         static_cast<float>(_viewport.width),
                                         static_cast<float>(_viewport.height),
                                         0.0f,  // minDepth
                                         1.0f); // maxDepth

        // Set scissor rect to match viewport
        wgpuRenderPassEncoderSetScissorRect(pass,
                                            _viewport.x,
                                            _viewport.y,
                                            _viewport.width,
                                            _viewport.height);

        // Bind uniform buffer if set
        if (_current_bind_group)
        {
            wgpuRenderPassEncoderSetBindGroup(pass, 0, _current_bind_group, 0, nullptr);
        }

        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, wgpu_vertex_buffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(pass, wgpu_index_buffer, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);

        // Draw indexed geometry
        wgpuRenderPassEncoderDrawIndexed(pass, index_count, 1, 0, 0, 0);

        // End render pass
        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        // Submit commands
        WGPUCommandBufferDescriptor cmd_buffer_desc = {};
        cmd_buffer_desc.nextInChain                 = nullptr;
        cmd_buffer_desc.label                       = make_string_view("Draw Command Buffer");
        WGPUCommandBuffer command                   = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        wgpuQueueSubmit(_queue, 1, &command);

        // Cleanup command resources
        wgpuCommandBufferRelease(command);
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(backbuffer_view);

        // Store surface texture to release after present
        _current_surface_texture = surface_texture.texture;

        return {};
    }

    void WgpuDevice::clear()
    {
        if (!_surface || !_queue)
        {
            return;
        }

        // Get current texture from surface
        WGPUSurfaceTexture surface_texture;
        wgpuSurfaceGetCurrentTexture(_surface, &surface_texture);

        if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
            surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
        {
            spdlog::error("Failed to get surface texture: {}", static_cast<int>(surface_texture.status));
            return;
        }

        // Create texture view
        WGPUTextureViewDescriptor view_desc = {};
        view_desc.nextInChain               = nullptr;
        view_desc.label                     = make_string_view("Surface Texture View");
        view_desc.format                    = _swapchain_format;
        view_desc.dimension                 = WGPUTextureViewDimension_2D;
        view_desc.baseMipLevel              = 0;
        view_desc.mipLevelCount             = 1;
        view_desc.baseArrayLayer            = 0;
        view_desc.arrayLayerCount           = 1;
        view_desc.aspect                    = WGPUTextureAspect_All;

        WGPUTextureView backbuffer_view = wgpuTextureCreateView(surface_texture.texture, &view_desc);

        // Create command encoder
        WGPUCommandEncoderDescriptor encoder_desc = {};
        encoder_desc.nextInChain                  = nullptr;
        encoder_desc.label                        = make_string_view("Clear Command Encoder");
        WGPUCommandEncoder encoder                = wgpuDeviceCreateCommandEncoder(_device, &encoder_desc);

        // Create render pass for clearing
        WGPURenderPassColorAttachment color_attachment = {};
        color_attachment.view                          = backbuffer_view;
        color_attachment.depthSlice                    = WGPU_DEPTH_SLICE_UNDEFINED;
        color_attachment.resolveTarget                 = nullptr;
        color_attachment.loadOp                        = WGPULoadOp_Clear;
        color_attachment.storeOp                       = WGPUStoreOp_Store;
        color_attachment.clearValue                    = { 0.1, 0.2, 0.3, 1.0 }; // Dark blue-gray

        WGPURenderPassDescriptor render_pass_desc = {};
        render_pass_desc.nextInChain              = nullptr;
        render_pass_desc.label                    = make_string_view("Clear Render Pass");
        render_pass_desc.colorAttachmentCount     = 1;
        render_pass_desc.colorAttachments         = &color_attachment;
        render_pass_desc.depthStencilAttachment   = nullptr;

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        // Submit commands
        WGPUCommandBufferDescriptor cmd_buffer_desc = {};
        cmd_buffer_desc.nextInChain                 = nullptr;
        cmd_buffer_desc.label                       = make_string_view("Clear Command Buffer");
        WGPUCommandBuffer command                   = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        wgpuQueueSubmit(_queue, 1, &command);

        // Cleanup command resources
        wgpuCommandBufferRelease(command);
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(backbuffer_view);

        // Store surface texture to release after present
        _current_surface_texture = surface_texture.texture;
    }

    void WgpuDevice::present()
    {
        if (!_surface)
        {
            return;
        }

        // Present the surface
        wgpuSurfacePresent(_surface);

        // Now release the surface texture after presenting
        if (_current_surface_texture)
        {
            wgpuTextureRelease(_current_surface_texture);
            _current_surface_texture = nullptr;
        }
    }

    std::expected<void, std::error_code>
    WgpuDevice::resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            spdlog::error("Invalid resize dimensions: {}x{}", width, height);
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        if (!_surface || !_device)
        {
            spdlog::error("Cannot resize: surface or device not initialized");
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Update stored dimensions
        _swapchain_width  = width;
        _swapchain_height = height;

        // Calculate viewport based on aspect ratio
        _viewport = calculate_viewport(width, height, _aspect_ratio, _custom_aspect_ratio);

        // Reconfigure surface with new dimensions
        WGPUSurfaceConfiguration surface_config = {};
        surface_config.nextInChain              = nullptr;
        surface_config.device                   = _device;
        surface_config.format                   = _swapchain_format;
        surface_config.usage                    = WGPUTextureUsage_RenderAttachment;
        surface_config.width                    = _swapchain_width;
        surface_config.height                   = _swapchain_height;
        surface_config.presentMode              = WGPUPresentMode_Fifo; // VSync
        surface_config.alphaMode                = WGPUCompositeAlphaMode_Auto;

        wgpuSurfaceConfigure(_surface, &surface_config);

        spdlog::info("Surface resized to {}x{}, viewport {}x{}",
                     width,
                     height,
                     _viewport.width,
                     _viewport.height);
        return {};
    }

    void WgpuDevice::set_aspect_ratio(AspectRatio ratio, float custom_value)
    {
        _aspect_ratio        = ratio;
        _custom_aspect_ratio = custom_value;

        // Recalculate viewport with new aspect ratio if surface is initialized
        if (_swapchain_width > 0 && _swapchain_height > 0)
        {
            _viewport = calculate_viewport(_swapchain_width, _swapchain_height, _aspect_ratio, _custom_aspect_ratio);
            spdlog::info("Aspect ratio changed, viewport updated to {}x{} at ({}, {})",
                         _viewport.width,
                         _viewport.height,
                         _viewport.x,
                         _viewport.y);
        }
    }

} // namespace raktr::render::backend
