/*!
 * @file wgpu_device.h
 * @brief WebGPU device implementation wrapping wgpu-native.
 */

#ifndef RAKTR_RENDER_WGPU_DEVICE_H
#define RAKTR_RENDER_WGPU_DEVICE_H

#include "aspect_ratio.h"
#include "bind_group.h"
#include "buffer.h"
#include "command_encoder.h"
#include "compute_pipeline.h"
#include "device.h"
#include "queue.h"
#include "render_pipeline.h"
#include "shader_module.h"
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
     * Wraps WGPUDevice, WGPUQueue, and WGPUSwapChain. Supports all device capabilities.
     */
    class WgpuDevice
    {
    public:
        /*!
         * @brief Create a WebGPU device with window surface.
         * @param window Window for creating surface and swapchain.
         * @param enable_validation Enable validation layers for debugging.
         * @return Device instance or error code.
         */
        [[nodiscard]] static std::expected<WgpuDevice, std::error_code>
        create(Window* window, bool enable_validation = false);

        ~WgpuDevice();

        // Disable copy (WebGPU handles cannot be safely copied)
        WgpuDevice(const WgpuDevice&)            = delete;
        WgpuDevice& operator=(const WgpuDevice&) = delete;

        // Custom move operations to properly transfer ownership
        WgpuDevice(WgpuDevice&& other) noexcept;
        WgpuDevice& operator=(WgpuDevice&& other) noexcept;

        // Buffer operations
        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_vertex_buffer(std::span<const std::byte> data);

        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_index_buffer(std::span<const std::byte> data);

        [[nodiscard]] std::expected<void, std::error_code>
        draw_indexed(const Buffer& vertex_buffer,
                     const Buffer& index_buffer,
                     uint32_t      index_count);

        void clear();
        void present();

        /*!
         * @brief Create a uniform buffer for shader constants.
         * @param size Size of the uniform buffer in bytes.
         * @return Buffer handle or error code.
         */
        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_uniform_buffer(size_t size);

        /*!
         * @brief Update uniform buffer data.
         * @param buffer Buffer handle from create_uniform_buffer().
         * @param data Data to upload.
         * @return Success or error code.
         */
        [[nodiscard]] std::expected<void, std::error_code>
        update_uniform_buffer(const Buffer& buffer, std::span<const std::byte> data);

        /*!
         * @brief Set uniform buffer for rendering.
         * Must be called before draw_indexed() to bind uniforms.
         * @param buffer Uniform buffer to bind.
         */
        void set_uniform_buffer(const Buffer& buffer);

        /*!
         * @brief Create an instance buffer for per-instance data.
         * @param data Instance data to upload (typically array of InstanceData).
         * @return Buffer handle or error code.
         */
        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_instance_buffer(std::span<const std::byte> data);

        /*!
         * @brief Update instance buffer data.
         * @param buffer Buffer handle from create_instance_buffer().
         * @param data New instance data to upload.
         * @return Success or error code.
         */
        [[nodiscard]] std::expected<void, std::error_code>
        update_instance_buffer(const Buffer& buffer, std::span<const std::byte> data);

        /*!
         * @brief Draw indexed geometry with instancing.
         * @param vertex_buffer Vertex buffer handle.
         * @param index_buffer Index buffer handle.
         * @param instance_buffer Instance buffer handle.
         * @param index_count Number of indices to draw.
         * @param instance_count Number of instances to render.
         * @return Success or error code.
         */
        [[nodiscard]] std::expected<void, std::error_code>
        draw_indexed_instanced(const Buffer& vertex_buffer,
                               const Buffer& index_buffer,
                               const Buffer& instance_buffer,
                               uint32_t      index_count,
                               uint32_t      instance_count);

        /*!
         * @brief Create a Hi-Z buffer for GPU-accelerated occlusion culling.
         *
         * Creates a Hierarchical Z-Buffer (depth pyramid) used for efficient visibility
         * testing of large numbers of objects. Uses compute shaders to build the pyramid
         * and test AABBs against the depth buffer.
         *
         * @param width Depth buffer width in pixels (must match render target).
         * @param height Depth buffer height in pixels (must match render target).
         * @return Hi-Z buffer instance or error code.
         *
         * @note Requires compute shader support. Width and height should match the
         *       depth pre-pass resolution for accurate culling.
         *
         * @example
         * auto hi_z = device->create_hi_z_buffer(1920, 1080);
         * if (hi_z) {
         *     hi_z.value()->build_pyramid(depth_texture);
         *     auto visible = hi_z.value()->test_visibility(aabbs, view_projection);
         * }
         */
        [[nodiscard]] std::expected<std::unique_ptr<occlusion::HiZBuffer>, std::error_code>
        create_hi_z_buffer(uint32_t width, uint32_t height);

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
        [[nodiscard]] std::expected<void, std::error_code>
        resize(uint32_t width, uint32_t height);

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
        void set_aspect_ratio(AspectRatio ratio, float custom_value = 1.0f);

        /*!
         * @brief Get the current aspect ratio setting.
         * @return Current aspect ratio mode.
         */
        [[nodiscard]] AspectRatio aspect_ratio() const
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
        const Viewport& viewport() const
        {
            return _viewport;
        }

        /*!
         * @brief Get the underlying WGPUInstance handle.
         * @return WGPUInstance handle (may be null if not initialized).
         */
        [[nodiscard]] WGPUInstance wgpu_instance() const
        {
            return _instance;
        }

        /*!
         * @brief Get the underlying WGPUDevice handle.
         * @return WGPUDevice handle (may be null if not initialized).
         */
        [[nodiscard]] WGPUDevice wgpu_device() const
        {
            return _device;
        }

        /*!
         * @brief Get the underlying WGPUQueue handle.
         * @return WGPUQueue handle (may be null if not initialized).
         */
        [[nodiscard]] WGPUQueue wgpu_queue() const
        {
            return _queue;
        }

        /*!
         * @brief Get the depth texture for Hi-Z buffer updates.
         * @return WGPUTexture handle (may be null if not initialized).
         */
        [[nodiscard]] WGPUTexture wgpu_depth_texture() const
        {
            return _depth_texture;
        }

        /*!
         * @brief Create a command encoder for recording GPU commands.
         * @return WGPUCommandEncoder handle (caller must release).
         * @note Used for RenderGraph PassContext setup.
         */
        [[nodiscard]] WGPUCommandEncoder wgpu_create_command_encoder() const;

        /*!
         * @brief Get the current surface texture view for rendering.
         * @return WGPUTextureView handle (valid until present()).
         * @note Used for RenderGraph PassContext color_target.
         */
        [[nodiscard]] WGPUTextureView wgpu_surface_texture_view();

        /*!
         * @brief Get the depth texture view for rendering.
         * @return WGPUTextureView handle (may be null if not initialized).
         * @note Used for RenderGraph PassContext depth_target.
         */
        [[nodiscard]] WGPUTextureView wgpu_depth_texture_view() const
        {
            return _depth_texture_view;
        }

        /*!
         * @brief Get the default render pipeline.
         * @return WGPURenderPipeline handle (may be null if not initialized).
         * @note Used by passes that need the default instancing-enabled pipeline.
         */
        [[nodiscard]] WGPURenderPipeline wgpu_render_pipeline() const
        {
            return _render_pipeline;
        }

        /*!
         * @brief Get the current bind group (uniforms).
         * @return WGPUBindGroup handle (may be null if not initialized).
         * @note Used by passes that need to bind uniform buffers.
         */
        [[nodiscard]] WGPUBindGroup wgpu_current_bind_group() const
        {
            return _current_bind_group;
        }

        /*!
         * @brief Get the device's queue for submitting commands.
         * @return Type-erased Queue wrapper.
         */
        [[nodiscard]] Queue queue();

        /*!
         * @brief Create a command encoder for recording GPU operations.
         * @param label Debug label for the encoder (optional).
         * @return Type-erased CommandEncoder wrapper.
         */
        [[nodiscard]] CommandEncoder create_command_encoder(std::string_view label = "");

        /*!
         * @brief Get current surface texture view for rendering.
         * @return Opaque pointer to surface texture view.
         */
        [[nodiscard]] void* get_surface_view();

        /*!
         * @brief Get current depth texture view for rendering.
         * @return Opaque pointer to depth texture view.
         */
        [[nodiscard]] void* get_depth_view() const;

        // Phase 3: Shader and Pipeline Creation
        /*!
         * @brief Create a shader module from WGSL source code.
         * @param descriptor Shader module descriptor with source code.
         * @return Type-erased ShaderModule or error code.
         */
        [[nodiscard]] std::expected<ShaderModule, std::error_code>
        create_shader_module(const ShaderModuleDescriptor& descriptor);

        /*!
         * @brief Create a render pipeline with vertex layout and shaders.
         * @param descriptor Render pipeline descriptor.
         * @return Type-erased RenderPipeline or error code.
         */
        [[nodiscard]] std::expected<RenderPipeline, std::error_code>
        create_render_pipeline(const RenderPipelineDescriptor& descriptor);

        /*!
         * @brief Create a compute pipeline.
         * @param descriptor Compute pipeline descriptor.
         * @return Type-erased ComputePipeline or error code.
         */
        [[nodiscard]] std::expected<ComputePipeline, std::error_code>
        create_compute_pipeline(const ComputePipelineDescriptor& descriptor);

        /*!
         * @brief Create a bind group layout.
         * @param descriptor Bind group layout descriptor.
         * @return Type-erased BindGroupLayout or error code.
         */
        [[nodiscard]] std::expected<BindGroupLayout, std::error_code>
        create_bind_group_layout(const BindGroupLayoutDescriptor& descriptor);

        /*!
         * @brief Create a bind group.
         * @param descriptor Bind group descriptor.
         * @return Type-erased BindGroup or error code.
         */
        [[nodiscard]] std::expected<BindGroup, std::error_code>
        create_bind_group(const BindGroupDescriptor& descriptor);

    private:
        WgpuDevice() = default;

        std::expected<void, std::error_code>
        initialize(Window* window, bool enable_validation);

        std::expected<void, std::error_code>
        create_shader_module(const char* wgsl_source, const char* label);

        std::expected<void, std::error_code>
        create_render_pipeline();

        std::expected<void, std::error_code>
        create_depth_texture();

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

        // Command buffer management
        std::vector<WGPUCommandBuffer> _command_buffers;

        // Shader and pipeline resources (legacy - for backward compatibility)
        WGPUShaderModule   _shader_module   = nullptr;
        WGPURenderPipeline _render_pipeline = nullptr;

        // Phase 3: Resource storage (ID-based handle management)
        std::vector<WGPUShaderModule>    _shader_modules;
        std::vector<WGPURenderPipeline>  _render_pipelines;
        std::vector<WGPUComputePipeline> _compute_pipelines;
        std::vector<WGPUBindGroupLayout> _bind_group_layouts;
        std::vector<WGPUBindGroup>       _bind_groups;

        // Uniform buffer and bind group management
        WGPUBindGroupLayout _bind_group_layout  = nullptr;
        WGPUBindGroup       _current_bind_group = nullptr;
        Buffer              _default_uniform_buffer;  // Identity matrix for backward compatibility
        Buffer              _default_instance_buffer; // Single identity instance for non-instanced rendering

        // Depth buffer
        WGPUTexture     _depth_texture      = nullptr;
        WGPUTextureView _depth_texture_view = nullptr;

        // Current frame surface texture (needs to be released after present)
        WGPUTexture     _current_surface_texture      = nullptr;
        WGPUTextureView _current_surface_texture_view = nullptr;
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_WGPU_DEVICE_H
