/*!
 * @file wgpu_backend.cpp
 * @brief Implementation of WgpuBackend.
 */

#include "backend/wgpu/wgpu_backend.h"
#include "render_error.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend
{

    WgpuBackend::WgpuBackend()
        : _window(nullptr), _wgpu_device(nullptr), _device(std::nullopt)
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
        // Create Device wrapper (non-owning, pointer = observer pattern)
        _device = Device(_wgpu_device.get());

        spdlog::info("WebGPU backend initialized successfully");
        return {};
    }

    void WgpuBackend::shutdown()
    {
        _device = std::nullopt;
        _wgpu_device.reset();
        _window.reset();
    }

    Device* WgpuBackend::device()
    {
        return _device.has_value() ? &_device.value() : nullptr;
    }

    void* WgpuBackend::backend_device_ptr()
    {
        return _wgpu_device.get();
    }

} // namespace raktr::render::backend
