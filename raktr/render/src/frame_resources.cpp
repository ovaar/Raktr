/*!
 * @file frame_resources.cpp
 * @brief Implementation of FrameResources.
 */

#include "frame_resources.h"
#include <cassert>
#include <webgpu/webgpu.h>


namespace raktr::render
{

    FrameResources::FrameResources(WGPUDevice device, uint32_t num_frames)
        : _device(device), _current_index(0), _frame_counter(0)
    {
        assert(device != nullptr && "Device cannot be null");
        assert(num_frames >= 2 && num_frames <= 3 && "Frame count must be 2 or 3");

        _frames.resize(num_frames);

        // Initialize resources for each frame
        // Note: Actual texture/buffer creation deferred until recreate_resources()
        // is called with actual dimensions
    }

    FrameResources::~FrameResources()
    {
        // Release all WGPU resources
        for (auto& frame : _frames)
        {
            if (frame.hi_z_pyramid != nullptr)
            {
                wgpuTextureRelease(frame.hi_z_pyramid);
                frame.hi_z_pyramid = nullptr;
            }

            if (frame.uniform_buffer != nullptr)
            {
                wgpuBufferRelease(frame.uniform_buffer);
                frame.uniform_buffer = nullptr;
            }
        }
    }

    FrameResource& FrameResources::current()
    {
        return _frames[_current_index];
    }

    const FrameResource& FrameResources::previous() const
    {
        // For first frame, return current (no previous data yet)
        if (_frame_counter == 0)
        {
            return _frames[_current_index];
        }

        // Ring buffer: previous = (current - 1 + size) % size
        size_t prev_index = (_current_index + _frames.size() - 1) % _frames.size();
        return _frames[prev_index];
    }

    void FrameResources::advance_frame()
    {
        _current_index = (_current_index + 1) % _frames.size();
        _frame_counter++;
    }

    void FrameResources::recreate_resources(uint32_t width, uint32_t height)
    {
        // Release old resources
        for (auto& frame : _frames)
        {
            if (frame.hi_z_pyramid != nullptr)
            {
                wgpuTextureRelease(frame.hi_z_pyramid);
                frame.hi_z_pyramid = nullptr;
            }

            if (frame.uniform_buffer != nullptr)
            {
                wgpuBufferRelease(frame.uniform_buffer);
                frame.uniform_buffer = nullptr;
            }
        }

        // Create Hi-Z pyramid textures
        // Calculate mip levels: log2(max(width, height))
        uint32_t max_dim    = (width > height) ? width : height;
        uint32_t mip_levels = 1;
        while (max_dim > 1)
        {
            max_dim >>= 1;
            mip_levels++;
        }

        for (auto& frame : _frames)
        {
            // Create Hi-Z pyramid texture
            WGPUTextureDescriptor tex_desc{};
            tex_desc.usage                   = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
            tex_desc.dimension               = WGPUTextureDimension_2D;
            tex_desc.size.width              = width;
            tex_desc.size.height             = height;
            tex_desc.size.depthOrArrayLayers = 1;
            tex_desc.format                  = WGPUTextureFormat_R32Float;
            tex_desc.mipLevelCount           = mip_levels;
            tex_desc.sampleCount             = 1;
            tex_desc.viewFormatCount         = 0;
            tex_desc.viewFormats             = nullptr;

            frame.hi_z_pyramid = wgpuDeviceCreateTexture(_device, &tex_desc);

            // Create uniform buffer (256 bytes for view/proj matrices + misc)
            WGPUBufferDescriptor buf_desc{};
            buf_desc.usage            = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
            buf_desc.size             = 256;
            buf_desc.mappedAtCreation = false;

            frame.uniform_buffer = wgpuDeviceCreateBuffer(_device, &buf_desc);
        }
    }

    void FrameResources::wait_idle()
    {
        // For WebGPU, we don't have explicit fences like Vulkan
        // Instead, use device poll to ensure work completes
        // This is a simplified implementation; production code would use
        // proper fence tracking with wgpuQueueOnSubmittedWorkDone

        // Note: wgpuDevicePoll is not standard WebGPU but available in wgpu-native
        // For now, this is a placeholder that should be replaced with proper
        // async fence tracking when integrating with the actual renderer

        // TODO: Implement proper fence-based synchronization
        // For each frame:
        //   - Track fence value when work is submitted
        //   - Wait until GPU signals fence completion
    }

} // namespace raktr::render
