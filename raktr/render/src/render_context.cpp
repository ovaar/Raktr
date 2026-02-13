/*!
 * @file render_context.cpp
 * @brief Implementation of RenderContext.
 */

#include "render_context.h"
#include "backend/ibackend.h"
#include "render_graph.h"
#include <memory>

namespace raktr::render
{

    struct RenderContext::Impl
    {
        std::unique_ptr<backend::IBackend> backend;
        bool                               initialized = false;
    };

    RenderContext::RenderContext()
        : _impl(std::make_unique<Impl>())
    {
    }

    RenderContext::~RenderContext()
    {
        shutdown();
    }

    RenderContext::RenderContext(RenderContext&&) noexcept            = default;
    RenderContext& RenderContext::operator=(RenderContext&&) noexcept = default;

    std::expected<void, std::error_code> RenderContext::initialize(const RenderConfig& config)
    {
        if (_impl->initialized)
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Create backend based on type
        _impl->backend = backend::create_backend(config.backend);
        if (!_impl->backend)
        {
            return std::unexpected(make_error_code(RenderError::BackendNotSupported));
        }

        // Initialize the backend
        auto result = _impl->backend->initialize(config);
        if (!result)
        {
            _impl->backend.reset();
            return result;
        }

        _impl->initialized = true;
        return {};
    }

    std::expected<void, std::error_code> RenderContext::initialize(
        const RenderConfig& config,
        const WindowConfig& window_config)
    {
        if (_impl->initialized)
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Create backend based on type
        _impl->backend = backend::create_backend(config.backend);
        if (!_impl->backend)
        {
            return std::unexpected(make_error_code(RenderError::BackendNotSupported));
        }

        // Initialize the backend with window config
        auto result = _impl->backend->initialize(config, window_config);
        if (!result)
        {
            _impl->backend.reset();
            return result;
        }

        _impl->initialized = true;
        return {};
    }

    std::expected<void, std::error_code> RenderContext::initialize(
        const RenderConfig& config,
        Window*             window)
    {
        if (_impl->initialized)
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Create backend based on type
        _impl->backend = backend::create_backend(config.backend);
        if (!_impl->backend)
        {
            return std::unexpected(make_error_code(RenderError::BackendNotSupported));
        }

        // Initialize the backend with existing window
        auto result = _impl->backend->initialize(config, window);
        if (!result)
        {
            _impl->backend.reset();
            return result;
        }

        _impl->initialized = true;
        return {};
    }

    void RenderContext::shutdown()
    {
        if (_impl->initialized && _impl->backend)
        {
            _impl->backend->shutdown();
            _impl->backend.reset();
            _impl->initialized = false;
        }
    }

    DeviceView RenderContext::device() const
    {
        if (_impl->initialized && _impl->backend)
        {
            return _impl->backend->device();
        }
        return DeviceView();
    }

    void RenderContext::execute(RenderGraph& graph, bool present)
    {
        if (_impl->initialized && _impl->backend)
        {
            _impl->backend->execute(graph, present);
        }
    }

    bool RenderContext::is_initialized() const
    {
        return _impl->initialized;
    }

    void* RenderContext::backend_ptr() const
    {
        if (_impl->initialized && _impl->backend)
        {
            return _impl->backend->backend_device_ptr();
        }
        return nullptr;
    }

    std::unique_ptr<RenderContext> create_render_context()
    {
        return std::make_unique<RenderContext>();
    }

} // namespace raktr::render
