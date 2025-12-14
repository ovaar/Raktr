/*!
 * @file wgpu_backend.h
 * @brief WebGPU backend implementation using wgpu-native.
 */

#ifndef RAKTR_RENDER_BACKEND_WGPU_BACKEND_H
#define RAKTR_RENDER_BACKEND_WGPU_BACKEND_H

#include "backend/ibackend.h"
#include "backend/wgpu/wgpu_device.h"
#include "window/window.h"
#include <memory>
#include <optional>

namespace raktr::render::backend
{
    /*!
     * @brief WebGPU backend using wgpu-native library.
     *
     * Provides cross-platform graphics rendering through WebGPU API,
     * supporting OpenGL, Vulkan, DirectX 12, and Metal backends.
     */
    class WgpuBackend : public IBackend
    {
    public:
        WgpuBackend();
        ~WgpuBackend() override;

        std::expected<void, std::error_code> initialize(const RenderConfig& config) override;
        std::expected<void, std::error_code> initialize(
            const RenderConfig& config,
            const WindowConfig& window_config) override;
        std::expected<void, std::error_code> initialize(
            const RenderConfig& config,
            Window*             window) override;

        void    shutdown() override;
        Device* device() override;
        void*   backend_device_ptr() override;

    private:
        std::expected<void, std::error_code> initialize_device(
            const RenderConfig& config,
            Window*             window);

        std::unique_ptr<Window>     _window;
        std::unique_ptr<WgpuDevice> _wgpu_device; // Raw backend device for backend_device_ptr()
        std::optional<Device>       _device;      // Type-erased wrapper (references _wgpu_device)
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_BACKEND_WGPU_BACKEND_H
