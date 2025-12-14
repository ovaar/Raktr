/*!
 * @file wgpu_shader_module.h
 * @brief WebGPU shader module implementation.
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <webgpu/webgpu.h>

namespace raktr::render::backend::wgpu
{

    /*!
     * @brief WebGPU shader module backend.
     *
     * Wraps WGPUShaderModule and manages its lifetime.
     */
    class WgpuShaderModule
    {
    public:
        /*!
         * @brief Create shader module from WGSL source.
         * @param device WebGPU device
         * @param code WGSL shader source code
         * @param label Optional debug label
         */
        WgpuShaderModule(WGPUDevice device, std::string_view code, std::string_view label);

        /*!
         * @brief Destructor - releases shader module.
         */
        ~WgpuShaderModule();

        // Non-copyable
        WgpuShaderModule(const WgpuShaderModule&)            = delete;
        WgpuShaderModule& operator=(const WgpuShaderModule&) = delete;

        // Movable
        WgpuShaderModule(WgpuShaderModule&& other) noexcept;
        WgpuShaderModule& operator=(WgpuShaderModule&& other) noexcept;

        /*!
         * @brief Get native WebGPU shader module handle.
         */
        [[nodiscard]] void* native_handle() const
        {
            return _shader_module;
        }

        /*!
         * @brief Get WebGPU shader module handle.
         */
        [[nodiscard]] WGPUShaderModule handle() const
        {
            return _shader_module;
        }

    private:
        WGPUShaderModule _shader_module = nullptr;
    };

} // namespace raktr::render::backend::wgpu
