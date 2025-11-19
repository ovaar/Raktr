/*!
 * @file soft_backend.cpp
 * @brief Implementation of SoftBackend.
 */

#include "backend/soft/soft_backend.h"
#include <memory>

namespace raktr::render::backend
{

    SoftBackend::SoftBackend()
        : _device(std::nullopt)
    {
    }

    SoftBackend::~SoftBackend()
    {
        shutdown();
    }

    std::expected<void, std::error_code> SoftBackend::initialize(const RenderConfig& /* config */)
    {
        _device = Device(SoftDevice());
        return {};
    }

    std::expected<void, std::error_code> SoftBackend::initialize(
        const RenderConfig& /* config */,
        const WindowConfig& /* window_config */)
    {
        // SoftBackend is software rendering - doesn't use window
        _device = Device(SoftDevice());
        return {};
    }

    std::expected<void, std::error_code> SoftBackend::initialize(
        const RenderConfig& /* config */,
        Window* /* window */)
    {
        // SoftBackend is software rendering - doesn't use window
        _device = Device(SoftDevice());
        return {};
    }

    void SoftBackend::shutdown()
    {
        _device = std::nullopt;
    }

    Device* SoftBackend::device()
    {
        return _device.has_value() ? &_device.value() : nullptr;
    }

} // namespace raktr::render::backend
