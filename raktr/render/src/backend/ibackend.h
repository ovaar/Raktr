/*!
 * @file ibackend.h
 * @brief Internal interface for graphics backends.
 */

#ifndef RAKTR_RENDER_BACKEND_IBACKEND_H
#define RAKTR_RENDER_BACKEND_IBACKEND_H

#include "device.h"
#include "render_context.h"
#include <expected>
#include <memory>
#include <system_error>


namespace raktr::render::backend
{
    /*!
     * @brief Internal interface for graphics backends.
     * This is not exposed in the public API.
     */
    class IBackend
    {
    public:
        virtual ~IBackend() = default;

        /*!
         * @brief Initialize with default window (backend owns window).
         */
        virtual std::expected<void, std::error_code> initialize(const RenderConfig& config) = 0;

        /*!
         * @brief Initialize with custom window config (backend owns window).
         */
        virtual std::expected<void, std::error_code> initialize(
            const RenderConfig& config,
            const WindowConfig& window_config) = 0;

        /*!
         * @brief Initialize with existing window (caller owns window).
         */
        virtual std::expected<void, std::error_code> initialize(
            const RenderConfig& config,
            Window*             window) = 0;

        /*!
         * @brief Shutdown the backend.
         */
        virtual void shutdown() = 0;

        /*!
         * @brief Get the device abstraction.
         */
        virtual Device* device() = 0;
    };

    /*!
     * @brief Factory to create backend implementations.
     */
    std::unique_ptr<IBackend> create_backend(BackendType type);

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_BACKEND_IBACKEND_H
