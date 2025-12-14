/*!
 * @file device_capabilities.h
 * @brief Device capability interfaces for type-erased Device pattern.
 *
 * This file defines capability structs that group related device operations.
 * Devices can support different subsets of capabilities, avoiding the need
 * for unsupported method stubs or dynamic_cast checks.
 */

#ifndef RAKTR_RENDER_DEVICE_CAPABILITIES_H
#define RAKTR_RENDER_DEVICE_CAPABILITIES_H

#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <system_error>

namespace raktr::render
{
    class Buffer;
    class Queue;
    class CommandEncoder;
    enum class AspectRatio;
    struct Viewport;

    class ShaderModule;
    class RenderPipeline;
    class ComputePipeline;
    class BindGroupLayout;
    class BindGroup;
    struct ShaderModuleDescriptor;
    struct RenderPipelineDescriptor;
    struct ComputePipelineDescriptor;
    struct BindGroupLayoutDescriptor;
    struct BindGroupDescriptor;

    namespace occlusion
    {
        class HiZBuffer;
    }
} // namespace raktr::render

// Forward declaration isn't enough for std::unique_ptr in std::function - need complete type
#include "occlusion/hi_z_buffer.h"

namespace raktr::render
{

    namespace capabilities
    {
        /*!
         * @brief Buffer creation and management capability.
         *
         * Provides functions for creating vertex, index, and uniform buffers.
         * All GPU devices should support this capability.
         */
        struct BufferOps
        {
            std::function<std::expected<Buffer, std::error_code>(std::span<const std::byte>)>
                create_vertex_buffer;

            std::function<std::expected<Buffer, std::error_code>(std::span<const std::byte>)>
                create_index_buffer;

            std::function<std::expected<Buffer, std::error_code>(size_t)>
                create_uniform_buffer;

            std::function<std::expected<void, std::error_code>(const Buffer&, std::span<const std::byte>)>
                update_uniform_buffer;

            std::function<void(const Buffer&)>
                set_uniform_buffer;
        };

        /*!
         * @brief Drawing and rendering capability.
         *
         * Provides functions for submitting draw calls.
         * All rendering devices should support this capability.
         */
        struct DrawOps
        {
            std::function<std::expected<void, std::error_code>(const Buffer&, const Buffer&, uint32_t)>
                draw_indexed;

            std::function<void()>
                clear;
        };

        /*!
         * @brief Viewport and aspect ratio management capability.
         *
         * Provides functions for managing viewport dimensions and aspect ratios.
         * GPU devices with surface/window support should have this capability.
         */
        struct ViewportOps
        {
            std::function<std::expected<void, std::error_code>(uint32_t, uint32_t)>
                resize;

            std::function<void(AspectRatio, float)>
                set_aspect_ratio;

            std::function<AspectRatio()>
                aspect_ratio;

            std::function<const Viewport&()>
                viewport;
        };

        /*!
         * @brief Frame presentation capability.
         *
         * Provides functions for presenting rendered frames to a display.
         * GPU devices with swap chain support should have this capability.
         */
        struct PresentOps
        {
            std::function<void()>
                present;
        };

        /*!
         * @brief Instanced rendering capability.
         *
         * Provides functions for creating instance buffers and drawing multiple
         * instances of geometry in a single draw call.
         * GPU devices with instancing support should have this capability.
         */
        struct InstancingOps
        {
            std::function<std::expected<Buffer, std::error_code>(std::span<const std::byte>)>
                create_instance_buffer;

            std::function<std::expected<void, std::error_code>(const Buffer&, std::span<const std::byte>)>
                update_instance_buffer;

            std::function<std::expected<void, std::error_code>(const Buffer&, const Buffer&, const Buffer&, uint32_t, uint32_t)>
                draw_indexed_instanced;
        };

        /*!
         * @brief Occlusion culling capability.
         *
         * Provides Hierarchical Z-Buffer (Hi-Z) occlusion culling for GPU-accelerated
         * visibility determination of large object counts (10,000+).
         * GPU devices with compute shader support should have this capability.
         */
        struct OcclusionCullingOps
        {
            std::function<std::expected<std::unique_ptr<occlusion::HiZBuffer>, std::error_code>(uint32_t, uint32_t)>
                create_hi_z_buffer;

            /*!
             * @brief Get the device's depth texture handle for Hi-Z pyramid updates.
             * @return void* pointer to backend-specific depth texture (e.g., WGPUTexture).
             */
            std::function<void*()> get_depth_texture;
        };

        /*!
         * @brief Queue access capability.
         *
         * Provides access to the device's command queue for submitting commands
         * and writing buffer data directly from CPU.
         */
        struct QueueOps
        {
            std::function<Queue()> queue;
        };

        /*!
         * @brief Command encoder creation capability.
         *
         * Provides creation of command encoders for recording GPU operations.
         * Required for explicit command recording in modern graphics APIs.
         */
        struct CommandEncoderOps
        {
            std::function<CommandEncoder(std::string_view)> create_command_encoder;
        };

        /*!
         * @brief Shader module creation capability (Phase 3).
         *
         * Provides functions for creating shader modules from WGSL source code.
         * Modern graphics APIs (WebGPU, Vulkan, DX12) should support this capability.
         */
        struct ShaderOps
        {
            std::function<std::expected<ShaderModule, std::error_code>(const ShaderModuleDescriptor&)>
                create_shader_module;
        };

        /*!
         * @brief Graphics pipeline creation capability (Phase 3).
         *
         * Provides functions for creating immutable render pipeline state objects
         * with vertex layouts, shader stages, and rasterization state.
         */
        struct RenderPipelineOps
        {
            std::function<std::expected<RenderPipeline, std::error_code>(const RenderPipelineDescriptor&)>
                create_render_pipeline;
        };

        /*!
         * @brief Compute pipeline creation capability (Phase 3).
         *
         * Provides functions for creating compute pipelines for GPGPU operations.
         */
        struct ComputePipelineOps
        {
            std::function<std::expected<ComputePipeline, std::error_code>(const ComputePipelineDescriptor&)>
                create_compute_pipeline;
        };

        /*!
         * @brief Bind group creation capability (Phase 3).
         *
         * Provides functions for creating bind group layouts and bind groups
         * that describe resource bindings for shaders.
         */
        struct BindGroupOps
        {
            std::function<std::expected<BindGroupLayout, std::error_code>(const BindGroupLayoutDescriptor&)>
                create_bind_group_layout;

            std::function<std::expected<BindGroup, std::error_code>(const BindGroupDescriptor&)>
                create_bind_group;
        };

    } // namespace capabilities

} // namespace raktr::render

#endif // RAKTR_RENDER_DEVICE_CAPABILITIES_H
