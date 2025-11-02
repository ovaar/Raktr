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
    view.data = str;
    view.length = str ? strlen(str) : 0;
    return view;
}

std::expected<std::unique_ptr<WgpuDevice>, std::error_code>
WgpuDevice::create(Window* window, bool enable_validation)
{
    auto device = std::unique_ptr<WgpuDevice>(new WgpuDevice());
    
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
    instance_desc.nextInChain = nullptr;
    _instance = wgpuCreateInstance(&instance_desc);
    if (!_instance)
    {
        spdlog::error("Failed to create WebGPU instance");
        return std::unexpected(make_error_code(RenderError::InitializationFailed));
    }

    // Create surface from window
    _swapchain_width = window->width();
    _swapchain_height = window->height();

#ifdef _WIN32
    WGPUSurfaceSourceWindowsHWND surface_source = {};
    surface_source.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    surface_source.chain.next = nullptr;
    surface_source.hinstance = GetModuleHandle(nullptr);
    surface_source.hwnd = static_cast<HWND>(window->native_handle());

    WGPUSurfaceDescriptor surface_desc = {};
    surface_desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&surface_source);
    surface_desc.label = make_string_view("Main Window Surface");

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
    adapter_opts.nextInChain = nullptr;
    adapter_opts.compatibleSurface = _surface;
    adapter_opts.powerPreference = WGPUPowerPreference_HighPerformance;

    WGPURequestAdapterCallbackInfo adapter_callback = {};
    adapter_callback.mode = WGPUCallbackMode_WaitAnyOnly;
    adapter_callback.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, 
                                   WGPUStringView message, void* userdata1, void* /* userdata2 */) {
        if (status == WGPURequestAdapterStatus_Success && adapter) {
            *static_cast<WGPUAdapter*>(userdata1) = adapter;
        } else {
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
    device_desc.nextInChain = nullptr;
    device_desc.label = make_string_view("Primary Device");
    device_desc.requiredFeatureCount = 0;
    device_desc.requiredFeatures = nullptr;
    device_desc.requiredLimits = nullptr;
    device_desc.defaultQueue.label = make_string_view("Default Queue");

    WGPURequestDeviceCallbackInfo device_callback = {};
    device_callback.mode = WGPUCallbackMode_WaitAnyOnly;
    device_callback.callback = [](WGPURequestDeviceStatus status, WGPUDevice device,
                                  WGPUStringView message, void* userdata1, void* /* userdata2 */) {
        if (status == WGPURequestDeviceStatus_Success && device) {
            *static_cast<WGPUDevice*>(userdata1) = device;
        } else {
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
    surface_config.nextInChain = nullptr;
    surface_config.device = _device;
    surface_config.format = _swapchain_format;
    surface_config.usage = WGPUTextureUsage_RenderAttachment;
    surface_config.width = _swapchain_width;
    surface_config.height = _swapchain_height;
    surface_config.presentMode = WGPUPresentMode_Fifo; // VSync
    surface_config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(_surface, &surface_config);

    spdlog::info("WebGPU device initialized successfully ({}x{})", _swapchain_width, _swapchain_height);
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

    // Release WebGPU resources (no swapchain in new API)
    if (_queue) { wgpuQueueRelease(_queue); _queue = nullptr; }
    if (_device) { wgpuDeviceRelease(_device); _device = nullptr; }
    if (_surface) { wgpuSurfaceRelease(_surface); _surface = nullptr; }
    if (_adapter) { wgpuAdapterRelease(_adapter); _adapter = nullptr; }
    if (_instance) { wgpuInstanceRelease(_instance); _instance = nullptr; }
}

std::expected<Buffer, std::error_code>
WgpuDevice::create_vertex_buffer(std::span<const std::byte> data)
{
    if (data.empty() || !_device)
    {
        return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
    }

    WGPUBufferDescriptor buffer_desc = {};
    buffer_desc.nextInChain = nullptr;
    buffer_desc.label = make_string_view("Vertex Buffer");
    buffer_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    buffer_desc.size = data.size();
    buffer_desc.mappedAtCreation = false;

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
    buffer_desc.nextInChain = nullptr;
    buffer_desc.label = make_string_view("Index Buffer");
    buffer_desc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
    buffer_desc.size = data.size();
    buffer_desc.mappedAtCreation = false;

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

std::expected<void, std::error_code>
WgpuDevice::draw_indexed(const Buffer& /* vertex_buffer */,
                        const Buffer& /* index_buffer */,
                        uint32_t /* index_count */)
{
    // TODO: Implement rendering with shaders and pipeline
    spdlog::warn("draw_indexed not yet implemented for WebGPU");
    return std::unexpected(make_error_code(RenderError::InvalidOperation));
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
    view_desc.nextInChain = nullptr;
    view_desc.label = make_string_view("Surface Texture View");
    view_desc.format = _swapchain_format;
    view_desc.dimension = WGPUTextureViewDimension_2D;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = 1;
    view_desc.aspect = WGPUTextureAspect_All;

    WGPUTextureView backbuffer_view = wgpuTextureCreateView(surface_texture.texture, &view_desc);

    // Create command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    encoder_desc.nextInChain = nullptr;
    encoder_desc.label = make_string_view("Clear Command Encoder");
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_device, &encoder_desc);

    // Create render pass for clearing
    WGPURenderPassColorAttachment color_attachment = {};
    color_attachment.view = backbuffer_view;
    color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color_attachment.resolveTarget = nullptr;
    color_attachment.loadOp = WGPULoadOp_Clear;
    color_attachment.storeOp = WGPUStoreOp_Store;
    color_attachment.clearValue = {0.1, 0.2, 0.3, 1.0}; // Dark blue-gray

    WGPURenderPassDescriptor render_pass_desc = {};
    render_pass_desc.nextInChain = nullptr;
    render_pass_desc.label = make_string_view("Clear Render Pass");
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &color_attachment;
    render_pass_desc.depthStencilAttachment = nullptr;

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    // Submit commands
    WGPUCommandBufferDescriptor cmd_buffer_desc = {};
    cmd_buffer_desc.nextInChain = nullptr;
    cmd_buffer_desc.label = make_string_view("Clear Command Buffer");
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
    wgpuQueueSubmit(_queue, 1, &command);

    // Cleanup
    wgpuCommandBufferRelease(command);
    wgpuCommandEncoderRelease(encoder);
    wgpuTextureViewRelease(backbuffer_view);
    
    // CRITICAL: Release texture before presenting/destroying surface
    wgpuTextureRelease(surface_texture.texture);
}

void WgpuDevice::present()
{
    if (!_surface)
    {
        return;
    }

    wgpuSurfacePresent(_surface);
}

} // namespace raktr::render::backend
