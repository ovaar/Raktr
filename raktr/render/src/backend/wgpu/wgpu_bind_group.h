/*!
 * @file wgpu_bind_group.h
 * @brief WebGPU bind group implementation.
 */

#pragma once

#include "bind_group.h"
#include <cstdint>
#include <vector>
#include <webgpu/webgpu.h>

namespace raktr::render::backend::wgpu
{

    /*!
     * @brief WebGPU bind group layout backend.
     *
     * Wraps WGPUBindGroupLayout.
     */
    class WgpuBindGroupLayout
    {
    public:
        /*!
         * @brief Create bind group layout from descriptor.
         * @param device WebGPU device
         * @param descriptor Layout descriptor
         */
        WgpuBindGroupLayout(WGPUDevice device, const BindGroupLayoutDescriptor& descriptor);

        /*!
         * @brief Destructor - releases layout.
         */
        ~WgpuBindGroupLayout();

        // Non-copyable
        WgpuBindGroupLayout(const WgpuBindGroupLayout&)            = delete;
        WgpuBindGroupLayout& operator=(const WgpuBindGroupLayout&) = delete;

        // Movable
        WgpuBindGroupLayout(WgpuBindGroupLayout&& other) noexcept;
        WgpuBindGroupLayout& operator=(WgpuBindGroupLayout&& other) noexcept;

        /*!
         * @brief Get native WebGPU layout handle.
         */
        [[nodiscard]] void* native_handle() const
        {
            return _layout;
        }

        /*!
         * @brief Get WebGPU layout handle.
         */
        [[nodiscard]] WGPUBindGroupLayout handle() const
        {
            return _layout;
        }

    private:
        WGPUBindGroupLayout _layout = nullptr;
    };

    /*!
     * @brief WebGPU bind group backend.
     *
     * Wraps WGPUBindGroup with resource bindings.
     */
    class WgpuBindGroup
    {
    public:
        /*!
         * @brief Create bind group from descriptor.
         * @param device WebGPU device
         * @param descriptor Bind group descriptor
         * @param buffers Pointer to device's buffer vector for ID lookup
         */
        WgpuBindGroup(WGPUDevice device, const BindGroupDescriptor& descriptor, const std::vector<WGPUBuffer>* buffers);

        /*!
         * @brief Destructor - releases bind group.
         */
        ~WgpuBindGroup();

        // Non-copyable
        WgpuBindGroup(const WgpuBindGroup&)            = delete;
        WgpuBindGroup& operator=(const WgpuBindGroup&) = delete;

        // Movable
        WgpuBindGroup(WgpuBindGroup&& other) noexcept;
        WgpuBindGroup& operator=(WgpuBindGroup&& other) noexcept;

        /*!
         * @brief Get native WebGPU bind group handle.
         */
        [[nodiscard]] void* native_handle() const
        {
            return _bind_group;
        }

        /*!
         * @brief Get WebGPU bind group handle.
         */
        [[nodiscard]] WGPUBindGroup handle() const
        {
            return _bind_group;
        }

    private:
        WGPUBindGroup _bind_group = nullptr;
    };

} // namespace raktr::render::backend::wgpu
