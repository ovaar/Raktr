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
    enum class AspectRatio;
    struct Viewport;

    namespace occlusion
    {
        class HiZBuffer;
    }
}

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
        };

    } // namespace capabilities

} // namespace raktr::render

#endif // RAKTR_RENDER_DEVICE_CAPABILITIES_H
