/*!
 * @file mock_backend.cpp
 * @brief Implementation of MockBackend.
 */

#include "backend/mock_backend.h"
#include <memory>

namespace raktr::render::backend
{

MockBackend::MockBackend()
    : _device(nullptr)
{
}

MockBackend::~MockBackend()
{
    shutdown();
}

std::expected<void, std::error_code> MockBackend::initialize(const RenderConfig& /* config */)
{
    _device = std::make_unique<MockDevice>();
    return {};
}

void MockBackend::shutdown()
{
    _device.reset();
}

Device* MockBackend::device()
{
    return _device.get();
}

} // namespace raktr::render::backend
