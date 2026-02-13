/*!
 * @file soft_backend.h
 * @brief Software backend for testing - CPU-based renderer with real behavior.
 */

#ifndef RAKTR_RENDER_BACKEND_SOFT_SOFT_BACKEND_H
#define RAKTR_RENDER_BACKEND_SOFT_SOFT_BACKEND_H

#include "backend/ibackend.h"
#include "soft_device.h"
#include <memory>
#include <optional>

namespace raktr::render::backend
{
    /*!
     * @brief Software backend for testing - implements real CPU-based rendering.
     */
    class SoftBackend : public IBackend
    {
    public:
        SoftBackend();
        ~SoftBackend() override;

        [[nodiscard]] std::expected<void, std::error_code> initialize(const RenderConfig& config) override;
        [[nodiscard]] std::expected<void, std::error_code> initialize(
            const RenderConfig& config,
            const WindowConfig& window_config) override;
        [[nodiscard]] std::expected<void, std::error_code> initialize(
            const RenderConfig& config,
            Window*             window) override;

        void                     shutdown() override;
        void                     execute(RenderGraph& graph, bool blocking) override;
        [[nodiscard]] DeviceView device() override;
        void*                    backend_device_ptr() override;

    private:
        std::unique_ptr<SoftDevice> _soft_device;
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_BACKEND_SOFT_SOFT_BACKEND_H
