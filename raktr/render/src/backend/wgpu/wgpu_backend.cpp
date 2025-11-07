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
        : _window(nullptr), _device(nullptr)
    {
    }

    WgpuBackend::~WgpuBackend()
    {
        shutdown();
    }

    std::expected<void, std::error_code> WgpuBackend::initialize(const RenderConfig& config)
    {
        // Create window
        WindowConfig window_config;
        window_config.width      = 1280;
        window_config.height     = 720;
        window_config.title      = "Raktr - WebGPU";
        window_config.resizable  = true;
        window_config.fullscreen = false;

        auto window_result = create_window(window_config);
        if (!window_result)
        {
            spdlog::error("Failed to create window for WebGPU backend");
            return std::unexpected(window_result.error());
        }
        _window = std::move(window_result.value());

        // Create WebGPU device
        auto device_result = WgpuDevice::create(_window.get(), config.enable_validation);
        if (!device_result)
        {
            spdlog::error("Failed to create WebGPU device");
            _window.reset();
            return std::unexpected(device_result.error());
        }
        _device = std::move(device_result.value());

        spdlog::info("WebGPU backend initialized successfully");
        return {};
    }

    void WgpuBackend::shutdown()
    {
        _device.reset();
        _window.reset();
    }

    Device* WgpuBackend::device()
    {
        return _device.get();
    }

} // namespace raktr::render::backend
