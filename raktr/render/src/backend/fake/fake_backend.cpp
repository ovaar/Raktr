/*!
 * @file fake_backend.cpp
 * @brief Implementation of FakeBackend.
 */

#include "backend/fake/fake_backend.h"
#include <memory>

namespace raktr::render::backend
{

    FakeBackend::FakeBackend()
        : _device(nullptr)
    {
    }

    FakeBackend::~FakeBackend()
    {
        shutdown();
    }

    std::expected<void, std::error_code> FakeBackend::initialize(const RenderConfig& /* config */)
    {
        _device = std::make_unique<FakeDevice>();
        return {};
    }

    void FakeBackend::shutdown()
    {
        _device.reset();
    }

    Device* FakeBackend::device()
    {
        return _device.get();
    }

} // namespace raktr::render::backend
