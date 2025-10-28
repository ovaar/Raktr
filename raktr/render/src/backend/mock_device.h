/*!
 * @file mock_device.h
 * @brief Mock device implementation for testing.
 */

#ifndef RAKTR_RENDER_BACKEND_MOCK_DEVICE_H
#define RAKTR_RENDER_BACKEND_MOCK_DEVICE_H

#include "device.h"
#include <vector>
#include <cstdint>

namespace raktr::render::backend
{
    /*!
     * @brief Mock device for testing (no actual GPU calls).
     */
    class MockDevice : public Device
    {
    public:
        MockDevice() = default;
        ~MockDevice() override = default;

        std::expected<Buffer, std::error_code> 
        create_vertex_buffer(std::span<const std::byte> data) override;

        std::expected<Buffer, std::error_code> 
        create_index_buffer(std::span<const std::byte> data) override;

        std::expected<void, std::error_code>
        draw_indexed(const Buffer& vertex_buffer, 
                    const Buffer& index_buffer, 
                    uint32_t index_count) override;

        void clear() override;
        void present() override;

    private:
        uint64_t _next_buffer_id = 1;
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_BACKEND_MOCK_DEVICE_H
