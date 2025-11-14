/*!
 * @file fake_backend.cpp
 * @brief Implementation of FakeBackend.
 */

#include "backend/fake/fake_backend.h"
#include <memory>

namespace raktr::render::backend
{

    FakeBackend::FakeBackend()
        : _device(std::nullopt)
    {
    }

    FakeBackend::~FakeBackend()
    {
        shutdown();
    }

    std::expected<void, std::error_code> FakeBackend::initialize(const RenderConfig& /* config */)
    {
        _device = Device(FakeDevice());
        return {};
    }

    std::expected<void, std::error_code> FakeBackend::initialize(
        const RenderConfig& /* config */,
        const WindowConfig& /* window_config */)
    {
        // FakeBackend is software rendering - doesn't use window
        _device = Device(FakeDevice());
        return {};
    }

    std::expected<void, std::error_code> FakeBackend::initialize(
        const RenderConfig& /* config */,
        Window* /* window */)
    {
        // FakeBackend is software rendering - doesn't use window
        _device = Device(FakeDevice());
        return {};
    }

    void FakeBackend::shutdown()
    {
        _device = std::nullopt;
    }

    Device* FakeBackend::device()
    {
        return _device.has_value() ? &_device.value() : nullptr;
    }

} // namespace raktr::render::backend
