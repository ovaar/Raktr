/*!
 * @file wgpu_device.h
 * @brief WebGPU device implementation wrapping wgpu-native.
 */

#ifndef RAKTR_RENDER_WGPU_DEVICE_H
#define RAKTR_RENDER_WGPU_DEVICE_H

#include "device.h"
#include <webgpu/webgpu.h>
#include <memory>
#include <vector>

namespace raktr::render
{
    // Forward declaration
    class Window;
}

namespace raktr::render::backend
{

/*!
 * @brief WebGPU device implementation using wgpu-native.
 * 
 * Wraps WGPUDevice, WGPUQueue, and WGPUSwapChain to implement the Device interface.
 */
class WgpuDevice : public Device
{
public:
    /*!
     * @brief Create a WebGPU device with window surface.
     * @param window Window for creating surface and swapchain.
     * @param enable_validation Enable validation layers for debugging.
     * @return Device instance or error code.
     */
    static std::expected<std::unique_ptr<WgpuDevice>, std::error_code>
    create(Window* window, bool enable_validation = false);

    ~WgpuDevice() override;

    // Device interface implementation
    std::expected<Buffer, std::error_code>
    create_vertex_buffer(std::span<const std::byte> data) override;

    std::expected<Buffer, std::error_code>
    create_index_buffer(std::span<const std::byte> data) override;

    std::expected<void, std::error_code>
    draw_indexed(const Buffer& vertex_buffer,
                const Buffer& index_buffer,
                uint32_t index_count) override;

    void clear() override;
    void present() override;

    /*!
     * @brief Get the underlying WGPUDevice handle.
     * @return WGPUDevice handle (may be null if not initialized).
     */
    WGPUDevice wgpu_device() const { return _device; }

    /*!
     * @brief Get the underlying WGPUQueue handle.
     * @return WGPUQueue handle (may be null if not initialized).
     */
    WGPUQueue wgpu_queue() const { return _queue; }

private:
    WgpuDevice() = default;

    std::expected<void, std::error_code>
    initialize(Window* window, bool enable_validation);

    std::expected<void, std::error_code>
    create_shader_module(const char* wgsl_source, const char* label);

    std::expected<void, std::error_code>
    create_render_pipeline();

    void cleanup();

    // WebGPU handles
    WGPUInstance _instance = nullptr;
    WGPUAdapter _adapter = nullptr;
    WGPUDevice _device = nullptr;
    WGPUQueue _queue = nullptr;
    WGPUSurface _surface = nullptr;

    // Surface configuration
    uint32_t _swapchain_width = 0;
    uint32_t _swapchain_height = 0;
    WGPUTextureFormat _swapchain_format = WGPUTextureFormat_BGRA8Unorm;

    // Buffer management
    std::vector<WGPUBuffer> _buffers;

    // Shader and pipeline resources
    WGPUShaderModule _shader_module = nullptr;
    WGPURenderPipeline _render_pipeline = nullptr;

    // Current frame surface texture (needs to be released after present)
    WGPUTexture _current_surface_texture = nullptr;
};

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_WGPU_DEVICE_H
