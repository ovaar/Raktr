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
        void shutdown() override;
        Device* device() override;

    private:
        std::unique_ptr<Window> _window;
        std::unique_ptr<WgpuDevice> _device;
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_BACKEND_WGPU_BACKEND_H
