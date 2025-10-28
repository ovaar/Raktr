/*!
 * @file fake_backend.h
 * @brief Fake backend for testing - software renderer with real behavior.
 */

#ifndef RAKTR_RENDER_BACKEND_FAKE_BACKEND_H
#define RAKTR_RENDER_BACKEND_FAKE_BACKEND_H

#include "backend/ibackend.h"
#include "fake_device.h"
#include <memory>

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

        std::expected<void, std::error_code> initialize(const RenderConfig& config) override;
        void shutdown() override;
        Device* device() override;

    private:
        std::unique_ptr<FakeDevice> _device;
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_BACKEND_FAKE_BACKEND_H
