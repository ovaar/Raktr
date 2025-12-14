/*!
 * @file wgpu_shader_module.cpp
 * @brief WebGPU shader module implementation.
 */

#include "wgpu_shader_module.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend::wgpu
{

    WgpuShaderModule::WgpuShaderModule(WGPUDevice device, std::string_view code, std::string_view label)
    {
        WGPUShaderSourceWGSL wgsl_desc = {};
        wgsl_desc.chain.sType          = WGPUSType_ShaderSourceWGSL;
        wgsl_desc.chain.next           = nullptr;
        wgsl_desc.code                 = WGPUStringView{ code.data(), code.length() };

        WGPUShaderModuleDescriptor desc = {};
        desc.nextInChain                = reinterpret_cast<WGPUChainedStruct*>(&wgsl_desc);
        desc.label                      = label.empty() ? WGPUStringView{ nullptr, 0 } : WGPUStringView{ label.data(), label.length() };

        _shader_module = wgpuDeviceCreateShaderModule(device, &desc);

        if (!_shader_module)
        {
            spdlog::error("Failed to create shader module: {}", label);
        }
    }

    WgpuShaderModule::~WgpuShaderModule()
    {
        if (_shader_module)
        {
            wgpuShaderModuleRelease(_shader_module);
            _shader_module = nullptr;
        }
    }

    WgpuShaderModule::WgpuShaderModule(WgpuShaderModule&& other) noexcept
        : _shader_module(other._shader_module)
    {
        other._shader_module = nullptr;
    }

    WgpuShaderModule& WgpuShaderModule::operator=(WgpuShaderModule&& other) noexcept
    {
        if (this != &other)
        {
            if (_shader_module)
            {
                wgpuShaderModuleRelease(_shader_module);
            }
            _shader_module       = other._shader_module;
            other._shader_module = nullptr;
        }
        return *this;
    }

} // namespace raktr::render::backend::wgpu
