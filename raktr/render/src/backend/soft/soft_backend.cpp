/*!
 * @file soft_backend.cpp
 * @brief Implementation of SoftBackend.
 */

#include "backend/soft/soft_backend.h"
#include <memory>

namespace raktr::render::backend
{

    SoftBackend::SoftBackend()
        : _soft_device(nullptr)
    {
    }

    SoftBackend::~SoftBackend()
    {
        shutdown();
    }

    std::expected<void, std::error_code> SoftBackend::initialize(const RenderConfig& /* config */)
    {
        _soft_device = std::make_unique<SoftDevice>();
        return {};
    }

    std::expected<void, std::error_code> SoftBackend::initialize(
        const RenderConfig& /* config */,
        const WindowConfig& /* window_config */)
    {
        // SoftBackend is software rendering - doesn't use window
        _soft_device = std::make_unique<SoftDevice>();
        return {};
    }

    std::expected<void, std::error_code> SoftBackend::initialize(
        const RenderConfig& /* config */,
        Window* /* window */)
    {
        // SoftBackend is software rendering - doesn't use window
        _soft_device = std::make_unique<SoftDevice>();
        return {};
    }

    void SoftBackend::shutdown()
    {
        _soft_device.reset();
    }

    void SoftBackend::execute(RenderGraph& /*graph*/, bool /*blocking*/)
    {
        // No-op for now
    }

    DeviceView SoftBackend::device()
    {
        return _soft_device ? DeviceView(_soft_device.get()) : DeviceView();
    }

    void* SoftBackend::backend_device_ptr()
    {
        return _soft_device.get();
    }

} // namespace raktr::render::backend
