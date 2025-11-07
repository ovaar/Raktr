/*!
 * @file backend_factory.cpp
 * @brief Factory for creating backend implementations.
 */

#include "backend/fake_backend.h"
#include "backend/ibackend.h"
#include "backend/wgpu/wgpu_backend.h"

namespace raktr::render::backend
{

    std::unique_ptr<IBackend> create_backend(BackendType type)
    {
        switch (type)
        {
            case BackendType::Fake:
                return std::make_unique<FakeBackend>();

            case BackendType::WebGPU:
                return std::make_unique<WgpuBackend>();

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
