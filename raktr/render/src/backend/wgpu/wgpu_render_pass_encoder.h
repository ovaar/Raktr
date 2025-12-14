/*!
 * @file wgpu_render_pass.h
 * @brief WebGPU render pass encoder implementation.
 */

#ifndef RAKTR_RENDER_WGPU_RENDER_PASS_H
#define RAKTR_RENDER_WGPU_RENDER_PASS_H

#include "buffer.h"
#include <vector>
#include <webgpu/webgpu.h>

namespace raktr::render::backend
{
    /*!
     * @brief WebGPU render pass encoder implementation.
     *
     * Wraps WGPURenderPassEncoder for recording graphics commands.
     */
    class WgpuRenderPassEncoder
    {
    public:
        /*!
         * @brief Construct from WebGPU render pass encoder handle.
         * @param encoder WebGPU render pass encoder (takes ownership).
         * @param buffers Pointer to device's buffer storage.
         */
        WgpuRenderPassEncoder(WGPURenderPassEncoder          encoder,
                              const std::vector<WGPUBuffer>* buffers)
            : _encoder(encoder), _buffers(buffers)
        {
        }

        // Non-copyable
        WgpuRenderPassEncoder(const WgpuRenderPassEncoder&)            = delete;
        WgpuRenderPassEncoder& operator=(const WgpuRenderPassEncoder&) = delete;

        // Movable
        WgpuRenderPassEncoder(WgpuRenderPassEncoder&& other) noexcept
            : _encoder(other._encoder), _buffers(other._buffers)
        {
            other._encoder = nullptr;
        }

        WgpuRenderPassEncoder& operator=(WgpuRenderPassEncoder&& other) noexcept
        {
            if (this != &other)
            {
                cleanup();
                _encoder       = other._encoder;
                _buffers       = other._buffers;
                other._encoder = nullptr;
            }
            return *this;
        }

        ~WgpuRenderPassEncoder()
        {
            cleanup();
        }

        /*!
         * @brief Set vertex buffer for rendering.
         * @param slot Vertex buffer binding slot.
         * @param buffer Vertex buffer to bind.
         * @param offset Byte offset into buffer.
         * @param size Size of buffer region to bind (0 = entire buffer).
         */
        void set_vertex_buffer(uint32_t slot, render::Buffer buffer, uint64_t offset, uint64_t size) const;

        /*!
         * @brief Set index buffer for indexed rendering.
         * @param buffer Index buffer to bind.
         * @param offset Byte offset into buffer.
         * @param size Size of buffer region to bind (0 = entire buffer).
         */
        void set_index_buffer(render::Buffer buffer, uint64_t offset, uint64_t size) const;

        /*!
         * @brief Draw non-indexed geometry.
         * @param vertex_count Number of vertices to draw.
         * @param instance_count Number of instances to draw.
         * @param first_vertex Offset to first vertex.
         * @param first_instance Offset to first instance.
         */
        void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) const;

        /*!
         * @brief Draw indexed geometry.
         * @param index_count Number of indices to draw.
         * @param instance_count Number of instances to draw.
         * @param first_index Offset to first index.
         * @param base_vertex Vertex offset added to each index.
         * @param first_instance Offset to first instance.
         */
        void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t base_vertex, uint32_t first_instance) const;

        /*!
         * @brief End the render pass.
         */
        void end() const;

        /*!
         * @brief Get the underlying WebGPU render pass encoder.
         * @return WGPURenderPassEncoder handle.
         */
        [[nodiscard]] WGPURenderPassEncoder wgpu_encoder() const
        {
            return _encoder;
        }

    private:
        void cleanup()
        {
            if (_encoder)
            {
                // Note: Don't release here - render pass encoder is ended, not released
                // It's owned by the command encoder
                _encoder = nullptr;
            }
        }

        mutable WGPURenderPassEncoder  _encoder; // Mutable for recording
        const std::vector<WGPUBuffer>* _buffers; // Device's buffer storage
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_WGPU_RENDER_PASS_H
