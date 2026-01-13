/*!
 * @file wgpu_command_encoder.h
 * @brief WebGPU command encoder implementation.
 */

#ifndef RAKTR_RENDER_WGPU_COMMAND_ENCODER_H
#define RAKTR_RENDER_WGPU_COMMAND_ENCODER_H

#include "buffer.h"
#include "command_buffer.h"
#include "compute_pass_encoder.h"
#include "render_pass_encoder.h"
#include <vector>
#include <webgpu/webgpu.h>
namespace raktr::render::backend::wgpu
{
    /*!
     * @brief WebGPU command encoder implementation.
     *
     * Wraps WGPUCommandEncoder for recording GPU commands.
     */
    class WgpuCommandEncoder
    {
    public:
        /*!
         * @brief Construct from WebGPU command encoder handle.
         * @param encoder WebGPU command encoder (takes ownership).
         * @param buffers Pointer to device's buffer storage.
         * @param command_buffers Pointer to device's command buffer storage for registering finished commands.
         */
        WgpuCommandEncoder(WGPUCommandEncoder              encoder,
                           const std::vector<WGPUBuffer>*  buffers,
                           std::vector<WGPUCommandBuffer>* command_buffers)
            : _encoder(encoder), _buffers(buffers), _command_buffers(command_buffers)
        {
        }

        // Non-copyable
        WgpuCommandEncoder(const WgpuCommandEncoder&)            = delete;
        WgpuCommandEncoder& operator=(const WgpuCommandEncoder&) = delete;

        // Movable
        WgpuCommandEncoder(WgpuCommandEncoder&& other) noexcept
            : _encoder(other._encoder), _buffers(other._buffers), _command_buffers(other._command_buffers)
        {
            other._encoder = nullptr;
        }

        WgpuCommandEncoder& operator=(WgpuCommandEncoder&& other) noexcept
        {
            if (this != &other)
            {
                cleanup();
                _encoder         = other._encoder;
                _buffers         = other._buffers;
                _command_buffers = other._command_buffers;
                other._encoder   = nullptr;
            }
            return *this;
        }

        ~WgpuCommandEncoder()
        {
            cleanup();
        }

        /*!
         * @brief Begin a render pass for recording graphics commands.
         * @param descriptor Render pass descriptor.
         * @return Type-erased RenderPassEncoder.
         */
        [[nodiscard]] render::RenderPassEncoder begin_render_pass(const render::RenderPassDescriptor& descriptor) const;

        /*!
         * @brief Begin a compute pass for recording compute commands.
         * @param descriptor Compute pass descriptor.
         * @return Type-erased ComputePassEncoder.
         */
        [[nodiscard]] render::ComputePassEncoder begin_compute_pass(const render::ComputePassDescriptor& descriptor) const;

        /*!
         * @brief Copy data between GPU buffers.
         * @param source Source buffer.
         * @param source_offset Byte offset in source.
         * @param destination Destination buffer.
         * @param destination_offset Byte offset in destination.
         * @param size Number of bytes to copy.
         */
        void copy_buffer_to_buffer(render::Buffer source, uint64_t source_offset, render::Buffer destination, uint64_t destination_offset, uint64_t size) const;

        /*!
         * @brief Finish recording and create command buffer.
         * @return Command buffer handle.
         */
        [[nodiscard]] render::CommandBuffer finish() const;

        /*!
         * @brief Get the underlying WebGPU command encoder.
         * @return WGPUCommandEncoder handle.
         */
        [[nodiscard]] WGPUCommandEncoder wgpu_encoder() const
        {
            return _encoder;
        }

    private:
        void cleanup()
        {
            if (_encoder)
            {
                wgpuCommandEncoderRelease(_encoder);
                _encoder = nullptr;
            }
        }

        mutable WGPUCommandEncoder      _encoder;         // Mutable for finish()
        const std::vector<WGPUBuffer>*  _buffers;         // Device's buffer storage
        std::vector<WGPUCommandBuffer>* _command_buffers; // Device's command buffer storage
    };

} // namespace raktr::render::backend::wgpu

#endif // RAKTR_RENDER_WGPU_COMMAND_ENCODER_H
