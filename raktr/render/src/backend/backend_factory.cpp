/*!
 * @file backend_factory.cpp
 * @brief Factory for creating backend implementations.
 */

#include "backend/ibackend.h"
#include "backend/mock_backend.h"

namespace raktr::render::backend
{

std::unique_ptr<IBackend> create_backend(BackendType type)
{
    switch (type)
    {
        case BackendType::Mock:
            return std::make_unique<MockBackend>();
        
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
