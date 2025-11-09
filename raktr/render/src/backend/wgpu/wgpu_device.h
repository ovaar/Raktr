/*!
 * @file wgpu_device.h
 * @brief WebGPU device implementation wrapping wgpu-native.
 */

#ifndef RAKTR_RENDER_WGPU_DEVICE_H
#define RAKTR_RENDER_WGPU_DEVICE_H

#include "aspect_ratio.h"
#include "buffer.h"
#include "device.h"
#include <memory>
#include <vector>
#include <webgpu/webgpu.h>

namespace raktr::render
{
    // Forward declaration
    class Window;
} // namespace raktr::render

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
                     uint32_t      index_count) override;

        void clear() override;
        void present() override;

        /*!
         * @brief Create a uniform buffer for shader constants.
         * @param size Size of the uniform buffer in bytes.
         * @return Buffer handle or error code.
         */
        std::expected<Buffer, std::error_code>
        create_uniform_buffer(size_t size) override;

        /*!
         * @brief Update uniform buffer data.
         * @param buffer Buffer handle from create_uniform_buffer().
         * @param data Data to upload.
         * @return Success or error code.
         */
        std::expected<void, std::error_code>
        update_uniform_buffer(const Buffer& buffer, std::span<const std::byte> data) override;

        /*!
         * @brief Set uniform buffer for rendering.
         * Must be called before draw_indexed() to bind uniforms.
         * @param buffer Uniform buffer to bind.
         */
        void set_uniform_buffer(const Buffer& buffer) override;

        /*!
         * @brief Resize the surface to new dimensions.
         *
         * Reconfigures the WebGPU surface with new width and height.
         * Should be called when the window is resized.
         *
         * @param width New width in pixels (must be > 0).
         * @param height New height in pixels (must be > 0).
         * @return Success or error code.
         *
         * @example
         * // In window resize callback:
         * window->set_resize_callback([&device](uint32_t w, uint32_t h) {
         *     device->resize(w, h);
         * });
         */
        std::expected<void, std::error_code>
        resize(uint32_t width, uint32_t height) override;

        /*!
         * @brief Set the aspect ratio for rendering.
         *
         * Controls how the viewport maintains proportions when the window is resized.
         * Uses letterboxing (black bars top/bottom) or pillarboxing (black bars left/right)
         * to maintain the specified aspect ratio.
         *
         * @param ratio Desired aspect ratio (default: Ratio_16_9).
         * @param custom_value Custom ratio value (only used if ratio == Custom).
         *
         * @example
         * // Use 16:9 aspect ratio (most common)
         * device->set_aspect_ratio(AspectRatio::Ratio_16_9);
         *
         * // Use ultrawide 21:9
         * device->set_aspect_ratio(AspectRatio::Ratio_21_9);
         *
         * // Use custom cinema ratio
         * device->set_aspect_ratio(AspectRatio::Custom, 2.35f);
         *
         * // Allow free stretching (no constraint)
         * device->set_aspect_ratio(AspectRatio::Auto);
         */
        void set_aspect_ratio(AspectRatio ratio, float custom_value = 1.0f) override;

        /*!
         * @brief Get the current aspect ratio setting.
         * @return Current aspect ratio mode.
         */
        AspectRatio aspect_ratio() const override
        {
            return _aspect_ratio;
        }

        /*!
         * @brief Get the current viewport rectangle.
         *
         * Returns the viewport used for rendering, which may be smaller than
         * the window if aspect ratio preservation is enabled.
         *
         * @return Current viewport (x, y, width, height).
         */
        const Viewport& viewport() const override
        {
            return _viewport;
        }

        /*!
         * @brief Get the underlying WGPUDevice handle.
         * @return WGPUDevice handle (may be null if not initialized).
         */
        WGPUDevice wgpu_device() const
        {
            return _device;
        }

        /*!
         * @brief Get the underlying WGPUQueue handle.
         * @return WGPUQueue handle (may be null if not initialized).
         */
        WGPUQueue wgpu_queue() const
        {
            return _queue;
        }

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
        WGPUAdapter  _adapter  = nullptr;
        WGPUDevice   _device   = nullptr;
        WGPUQueue    _queue    = nullptr;
        WGPUSurface  _surface  = nullptr;

        // Surface configuration
        uint32_t          _swapchain_width  = 0;
        uint32_t          _swapchain_height = 0;
        WGPUTextureFormat _swapchain_format = WGPUTextureFormat_BGRA8Unorm;

        // Aspect ratio and viewport
        AspectRatio _aspect_ratio        = AspectRatio::Ratio_16_9; // Default to 16:9
        float       _custom_aspect_ratio = 1.0f;
        Viewport    _viewport            = {};

        // Buffer management
        std::vector<WGPUBuffer> _buffers;

        // Shader and pipeline resources
        WGPUShaderModule   _shader_module   = nullptr;
        WGPURenderPipeline _render_pipeline = nullptr;

        // Uniform buffer and bind group management
        WGPUBindGroupLayout _bind_group_layout  = nullptr;
        WGPUBindGroup       _current_bind_group = nullptr;
        Buffer              _default_uniform_buffer; // Identity matrix for backward compatibility

        // Current frame surface texture (needs to be released after present)
        WGPUTexture _current_surface_texture = nullptr;
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_WGPU_DEVICE_H
