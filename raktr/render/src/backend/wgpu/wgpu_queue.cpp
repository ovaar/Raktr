/*!
 * @file wgpu_queue.cpp
 * @brief WebGPU queue implementation.
 */

#include "wgpu_queue.h"
#include "wgpu_device.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend
{
    void WgpuQueue::submit(std::span<const render::CommandBuffer> commands) const
    {
        if (!_queue)
        {
            spdlog::error("WgpuQueue::submit: Invalid queue");
            return;
        }

        if (commands.empty())
        {
            return; // Nothing to submit
        }

        // Convert CommandBuffer handles to WGPUCommandBuffer
        std::vector<WGPUCommandBuffer> wgpu_commands;
        wgpu_commands.reserve(commands.size());

        for (const auto& cmd : commands)
        {
            if (!cmd.is_valid())
            {
                spdlog::warn("WgpuQueue::submit: Skipping invalid command buffer");
                continue;
            }

            if (cmd.id() >= _command_buffers->size())
            {
                spdlog::error("WgpuQueue::submit: Command buffer ID {} out of range", cmd.id());
                continue;
            }

            WGPUCommandBuffer wgpu_cmd = (*_command_buffers)[cmd.id()];
            if (wgpu_cmd)
            {
                wgpu_commands.push_back(wgpu_cmd);
            }
        }

        if (!wgpu_commands.empty())
        {
            wgpuQueueSubmit(_queue, static_cast<uint32_t>(wgpu_commands.size()), wgpu_commands.data());
            spdlog::debug("WgpuQueue: Submitted {} command buffers", wgpu_commands.size());
        }
    }

    void WgpuQueue::write_buffer(render::Buffer buffer, uint64_t offset, std::span<const std::byte> data) const
    {
        if (!_queue)
        {
            spdlog::error("WgpuQueue::write_buffer: Invalid queue");
            return;
        }

        if (!buffer.is_valid())
        {
            spdlog::error("WgpuQueue::write_buffer: Invalid buffer");
            return;
        }

        if (data.empty())
        {
            return; // Nothing to write
        }

        if (buffer.id() >= _buffers->size())
        {
            spdlog::error("WgpuQueue::write_buffer: Buffer ID {} out of range (max: {})",
                          buffer.id(),
                          _buffers->size());
            return;
        }

        WGPUBuffer wgpu_buffer = (*_buffers)[buffer.id()];
        if (!wgpu_buffer)
        {
            spdlog::error("WgpuQueue::write_buffer: Null buffer handle for ID {}", buffer.id());
            return;
        }

        wgpuQueueWriteBuffer(_queue, wgpu_buffer, offset, data.data(), data.size());

        spdlog::debug("WgpuQueue: Wrote {} bytes to buffer {} at offset {}",
                      data.size(),
                      buffer.id(),
                      offset);
    }

} // namespace raktr::render::backend
