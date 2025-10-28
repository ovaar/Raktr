/*!
 * @file mock_backend.h
 * @brief Mock backend for unit testing (no actual GPU operations).
 */

#ifndef RAKTR_RENDER_BACKEND_MOCK_BACKEND_H
#define RAKTR_RENDER_BACKEND_MOCK_BACKEND_H

#include "backend/ibackend.h"
#include "mock_device.h"
#include <memory>

namespace raktr::render::backend
{
    /*!
     * @brief Mock backend for unit testing (no actual GPU operations).
     */
    class MockBackend : public IBackend
    {
    public:
        MockBackend();
        ~MockBackend() override;

        std::expected<void, std::error_code> initialize(const RenderConfig& config) override;
        void shutdown() override;
        Device* device() override;

    private:
        std::unique_ptr<MockDevice> _device;
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_BACKEND_MOCK_BACKEND_H
