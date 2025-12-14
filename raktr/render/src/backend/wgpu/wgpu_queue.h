/*!
 * @file wgpu_queue.h
 * @brief WebGPU queue implementation.
 */

#ifndef RAKTR_RENDER_WGPU_QUEUE_H
#define RAKTR_RENDER_WGPU_QUEUE_H

#include "buffer.h"
#include "command_buffer.h"
#include <span>
#include <vector>
#include <webgpu/webgpu.h>

namespace raktr::render::backend
{
    /*!
     * @brief WebGPU queue implementation.
     *
     * Wraps WGPUQueue for submitting commands and writing buffer data.
     */
    class WgpuQueue
    {
    public:
        /*!
         * @brief Construct from WebGPU queue handle.
         * @param queue WebGPU queue handle (does not take ownership).
         * @param buffers Pointer to device's buffer storage.
         * @param command_buffers Pointer to device's command buffer storage.
         */
        WgpuQueue(WGPUQueue                             queue,
                  const std::vector<WGPUBuffer>*        buffers,
                  const std::vector<WGPUCommandBuffer>* command_buffers)
            : _queue(queue), _buffers(buffers), _command_buffers(command_buffers)
        {
        }

        /*!
         * @brief Submit command buffers for execution.
         * @param commands Span of command buffer handles to submit.
         */
        void submit(std::span<const render::CommandBuffer> commands) const;

        /*!
         * @brief Write data directly to a GPU buffer.
         * @param buffer Target buffer.
         * @param offset Byte offset into buffer.
         * @param data Data to write.
         */
        void write_buffer(render::Buffer buffer, uint64_t offset, std::span<const std::byte> data) const;

        /*!
         * @brief Get the underlying WebGPU queue handle.
         * @return WGPUQueue handle.
         */
        [[nodiscard]] WGPUQueue wgpu_queue() const
        {
            return _queue;
        }

    private:
        WGPUQueue                             _queue;           // WebGPU queue handle (not owned)
        const std::vector<WGPUBuffer>*        _buffers;         // Device's buffer storage
        const std::vector<WGPUCommandBuffer>* _command_buffers; // Device's command buffer storage
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_WGPU_QUEUE_H
