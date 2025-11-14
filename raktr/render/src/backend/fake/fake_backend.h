/*!
 * @file fake_backend.h
 * @brief Fake backend for testing - software renderer with real behavior.
 */

#ifndef RAKTR_RENDER_BACKEND_FAKE_FAKE_BACKEND_H
#define RAKTR_RENDER_BACKEND_FAKE_FAKE_BACKEND_H

#include "backend/ibackend.h"
#include "fake_device.h"
#include <memory>
#include <optional>

namespace raktr::render::backend
{
    /*!
     * @brief Fake backend for testing - implements real software rendering.
     */
    class FakeBackend : public IBackend
    {
    public:
        FakeBackend();
        ~FakeBackend() override;

        [[nodiscard]] std::expected<void, std::error_code> initialize(const RenderConfig& config) override;
        [[nodiscard]] std::expected<void, std::error_code> initialize(
            const RenderConfig& config,
            const WindowConfig& window_config) override;
        [[nodiscard]] std::expected<void, std::error_code> initialize(
            const RenderConfig& config,
            Window*             window) override;

        void                  shutdown() override;
        [[nodiscard]] Device* device() override;

    private:
        std::optional<Device> _device;
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_BACKEND_FAKE_FAKE_BACKEND_H
