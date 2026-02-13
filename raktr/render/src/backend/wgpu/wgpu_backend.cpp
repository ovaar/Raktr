/*!
 * @file wgpu_backend.cpp
 * @brief Implementation of WgpuBackend.
 */

#include "backend/wgpu/wgpu_backend.h"
#include "backend/wgpu/wgpu_pass_context.h"
#include "render_error.h"
#include "render_graph.h"
#include <spdlog/spdlog.h>
#include <webgpu/webgpu.h>

namespace raktr::render::backend::wgpu
{

    WgpuBackend::WgpuBackend()
        : _window(nullptr), _wgpu_device(nullptr)
    {
    }

    WgpuBackend::~WgpuBackend()
    {
        shutdown();
    }

    std::expected<void, std::error_code> WgpuBackend::initialize(const RenderConfig& config)
    {
        // Default window configuration
        WindowConfig default_config;
        default_config.width      = 1280;
        default_config.height     = 720;
        default_config.title      = "Raktr - WebGPU";
        default_config.resizable  = true;
        default_config.fullscreen = false;

        return initialize(config, default_config);
    }

    std::expected<void, std::error_code> WgpuBackend::initialize(
        const RenderConfig& config,
        const WindowConfig& window_config)
    {
        // Backend owns window
        auto window_result = create_window(window_config);
        if (!window_result)
        {
            spdlog::error("Failed to create window for WebGPU backend");
            return std::unexpected(window_result.error());
        }
        _window = std::move(window_result.value());
        return initialize_device(config, _window.get());
    }

    std::expected<void, std::error_code> WgpuBackend::initialize(
        const RenderConfig& config,
        Window*             window)
    {
        // Caller owns window
        if (!window)
        {
            spdlog::error("Provided window pointer is null");
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }
        return initialize_device(config, window);
    }

    std::expected<void, std::error_code> WgpuBackend::initialize_device(
        const RenderConfig& config,
        Window*             window)
    {
        // Shared device creation logic
        auto device_result = WgpuDevice::create(window, config.enable_validation);
        if (!device_result)
        {
            spdlog::error("Failed to create WebGPU device");
            _window.reset(); // Safe - only affects owned window
            return std::unexpected(device_result.error());
        }

        // Store device in unique_ptr
        _wgpu_device = std::make_unique<WgpuDevice>(std::move(device_result.value()));

        spdlog::info("WebGPU backend initialized successfully");
        return {};
    }

    void WgpuBackend::shutdown()
    {
        _wgpu_device.reset();
        _window.reset();
    }

    DeviceView WgpuBackend::device()
    {
        return _wgpu_device ? DeviceView(_wgpu_device.get()) : DeviceView();
    }

    void* WgpuBackend::backend_device_ptr()
    {
        return _wgpu_device.get();
    }

    void WgpuBackend::execute(RenderGraph& graph, bool present)
    {
        if (!_wgpu_device || !_window)
            return;

        WGPUTextureView depth_view = static_cast<WGPUTextureView>(_wgpu_device->get_depth_view());

        // Start frame if not active
        if (!_is_frame_active)
        {
            WGPUSurface surface = static_cast<WGPUSurface>(_wgpu_device->get_surface_view());
            wgpuSurfaceGetCurrentTexture(surface, &_current_surface_texture);

            if (_current_surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal)
            {
                return;
            }

            WGPUTextureViewDescriptor view_desc = {};
            view_desc.label                     = { "SurfaceTextureView", WGPU_STRLEN };
            view_desc.format                    = wgpuTextureGetFormat(_current_surface_texture.texture);
            view_desc.dimension                 = WGPUTextureViewDimension_2D;
            view_desc.baseMipLevel              = 0;
            view_desc.mipLevelCount             = 1;
            view_desc.baseArrayLayer            = 0;
            view_desc.arrayLayerCount           = 1;
            view_desc.aspect                    = WGPUTextureAspect_All;
            _current_color_view                 = wgpuTextureCreateView(_current_surface_texture.texture, &view_desc);

            _is_frame_active = true;
        }

        // Create encoder for this batch
        WGPUCommandEncoderDescriptor encoder_desc = {};
        encoder_desc.label                        = { "RenderGraph Encoder", WGPU_STRLEN };
        WGPUCommandEncoder encoder                = wgpuDeviceCreateCommandEncoder(_wgpu_device->_device, &encoder_desc);

        WgpuPassContext ctx;
        ctx.frame_index     = _frame_index;
        ctx.command_encoder = encoder;
        ctx.color_target    = _current_color_view;
        ctx.depth_target    = depth_view;
        ctx.viewport_width  = _window->width();
        ctx.viewport_height = _window->height();

        graph.execute(ctx);

        WGPUCommandBufferDescriptor cmd_buf_desc = {};
        WGPUCommandBuffer           cmd_buffer   = wgpuCommandEncoderFinish(encoder, &cmd_buf_desc);

        wgpuQueueSubmit(_wgpu_device->_queue, 1, &cmd_buffer);

        wgpuCommandBufferRelease(cmd_buffer);
        wgpuCommandEncoderRelease(encoder);

        if (present)
        {
            WGPUSurface surface = static_cast<WGPUSurface>(_wgpu_device->get_surface_view());
            wgpuSurfacePresent(surface);

            wgpuTextureViewRelease(_current_color_view);
            wgpuTextureRelease(_current_surface_texture.texture);

            _current_color_view      = nullptr;
            _current_surface_texture = {};
            _is_frame_active         = false;

            _frame_index++;
        }
    }

} // namespace raktr::render::backend::wgpu
