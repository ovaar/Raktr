/*!
 * @file backend_factory.cpp
 * @brief Factory for creating backend implementations.
 */

#include "backend/ibackend.h"
#include "backend/soft/soft_backend.h"
#include "backend/wgpu/wgpu_backend.h"

namespace raktr::render::backend
{

    std::unique_ptr<IBackend> create_backend(BackendType type)
    {
        switch (type)
        {
            case BackendType::Soft:
                return std::make_unique<SoftBackend>();

            case BackendType::WebGPU:
                return std::make_unique<wgpu::WgpuBackend>();

            case BackendType::OpenGL:
            case BackendType::Vulkan:
            case BackendType::DirectX12:
                // Not implemented yet
                return nullptr;

            default:
                return nullptr;
        }
    }

} // namespace raktr::render::backend
