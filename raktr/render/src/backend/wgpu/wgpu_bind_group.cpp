/*!
 * @file wgpu_bind_group.cpp
 * @brief WebGPU bind group implementation.
 */

#include "wgpu_bind_group.h"
#include <spdlog/spdlog.h>
#include <vector>

namespace raktr::render::backend::wgpu
{

    namespace
    {
        WGPUShaderStage convert_shader_stage(ShaderStage stage)
        {
            WGPUShaderStage result = WGPUShaderStage_None;

            uint8_t stage_bits = static_cast<uint8_t>(stage);
            if (stage_bits & static_cast<uint8_t>(ShaderStage::Vertex))
                result |= WGPUShaderStage_Vertex;
            if (stage_bits & static_cast<uint8_t>(ShaderStage::Fragment))
                result |= WGPUShaderStage_Fragment;
            if (stage_bits & static_cast<uint8_t>(ShaderStage::Compute))
                result |= WGPUShaderStage_Compute;

            return result;
        }

        WGPUBufferBindingType convert_binding_type(BindingType type)
        {
            switch (type)
            {
                case BindingType::UniformBuffer:
                    return WGPUBufferBindingType_Uniform;
                case BindingType::StorageBuffer:
                    return WGPUBufferBindingType_Storage;
                default:
                    return WGPUBufferBindingType_Uniform;
            }
        }
    } // namespace

    // ============================================================================
    // WgpuBindGroupLayout
    // ============================================================================

    WgpuBindGroupLayout::WgpuBindGroupLayout(WGPUDevice device, const BindGroupLayoutDescriptor& descriptor)
    {
        std::vector<WGPUBindGroupLayoutEntry> entries;
        entries.reserve(descriptor.entries.size());

        for (const auto& entry : descriptor.entries)
        {
            WGPUBindGroupLayoutEntry wgpu_entry = {};
            wgpu_entry.binding                  = entry.binding;
            wgpu_entry.visibility               = convert_shader_stage(entry.visibility);

            // Set buffer binding
            if (entry.type == BindingType::UniformBuffer || entry.type == BindingType::StorageBuffer)
            {
                wgpu_entry.buffer.type             = convert_binding_type(entry.type);
                wgpu_entry.buffer.hasDynamicOffset = false;
                wgpu_entry.buffer.minBindingSize   = 0;
            }
            // TODO: Add texture and sampler support in future phases

            entries.push_back(wgpu_entry);
        }

        WGPUBindGroupLayoutDescriptor layout_desc = {};
        layout_desc.label                         = descriptor.label.empty() ? WGPUStringView{ nullptr, 0 } : WGPUStringView{ descriptor.label.data(), descriptor.label.length() };
        layout_desc.entryCount                    = static_cast<uint32_t>(entries.size());
        layout_desc.entries                       = entries.data();

        _layout = wgpuDeviceCreateBindGroupLayout(device, &layout_desc);

        if (!_layout)
        {
            spdlog::error("Failed to create bind group layout: {}", descriptor.label);
        }
    }

    WgpuBindGroupLayout::~WgpuBindGroupLayout()
    {
        if (_layout)
        {
            wgpuBindGroupLayoutRelease(_layout);
            _layout = nullptr;
        }
    }

    WgpuBindGroupLayout::WgpuBindGroupLayout(WgpuBindGroupLayout&& other) noexcept
        : _layout(other._layout)
    {
        other._layout = nullptr;
    }

    WgpuBindGroupLayout& WgpuBindGroupLayout::operator=(WgpuBindGroupLayout&& other) noexcept
    {
        if (this != &other)
        {
            if (_layout)
            {
                wgpuBindGroupLayoutRelease(_layout);
            }
            _layout       = other._layout;
            other._layout = nullptr;
        }
        return *this;
    }

    // ============================================================================
    // WgpuBindGroup
    // ============================================================================

    WgpuBindGroup::WgpuBindGroup(WGPUDevice device, const BindGroupDescriptor& descriptor, const std::vector<WGPUBuffer>* buffers)
    {
        std::vector<WGPUBindGroupEntry> entries;
        entries.reserve(descriptor.entries.size());

        for (const auto& entry : descriptor.entries)
        {
            WGPUBindGroupEntry wgpu_entry = {};
            wgpu_entry.binding            = entry.binding;

            // Get buffer handle from ID
            if (entry.buffer.id() < buffers->size())
            {
                wgpu_entry.buffer = (*buffers)[entry.buffer.id()];
                wgpu_entry.offset = entry.offset;
                wgpu_entry.size   = entry.size;
            }
            else
            {
                spdlog::error("Invalid buffer ID {} in bind group entry", entry.buffer.id());
            }

            entries.push_back(wgpu_entry);
        }

        WGPUBindGroupDescriptor bind_group_desc = {};
        bind_group_desc.label                   = descriptor.label.empty() ? WGPUStringView{ nullptr, 0 } : WGPUStringView{ descriptor.label.data(), descriptor.label.length() };
        bind_group_desc.layout                  = static_cast<WGPUBindGroupLayout>(descriptor.layout.native_handle());
        bind_group_desc.entryCount              = static_cast<uint32_t>(entries.size());
        bind_group_desc.entries                 = entries.data();

        _bind_group = wgpuDeviceCreateBindGroup(device, &bind_group_desc);

        if (!_bind_group)
        {
            spdlog::error("Failed to create bind group: {}", descriptor.label);
        }
    }

    WgpuBindGroup::~WgpuBindGroup()
    {
        if (_bind_group)
        {
            wgpuBindGroupRelease(_bind_group);
            _bind_group = nullptr;
        }
    }

    WgpuBindGroup::WgpuBindGroup(WgpuBindGroup&& other) noexcept
        : _bind_group(other._bind_group)
    {
        other._bind_group = nullptr;
    }

    WgpuBindGroup& WgpuBindGroup::operator=(WgpuBindGroup&& other) noexcept
    {
        if (this != &other)
        {
            if (_bind_group)
            {
                wgpuBindGroupRelease(_bind_group);
            }
            _bind_group       = other._bind_group;
            other._bind_group = nullptr;
        }
        return *this;
    }

} // namespace raktr::render::backend::wgpu
