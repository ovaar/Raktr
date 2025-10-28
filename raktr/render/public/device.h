/*!
 * @file device.h
 * @brief GPU device abstraction for creating resources.
 */

#ifndef RAKTR_RENDER_DEVICE_H
#define RAKTR_RENDER_DEVICE_H

#include "buffer.h"
#include "render_error.h"
#include <expected>
#include <span>
#include <cstddef>
#include <system_error>

namespace raktr::render
{
    /*!
     * @brief GPU device abstraction for creating resources.
     */
    class Device
    {
    public:
        Device() = default;
        virtual ~Device() = default;

        // Non-copyable, non-moveable (abstract interface)
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        /*!
         * @brief Create a vertex buffer.
         * @param data Vertex data to upload.
         * @return Buffer handle or error.
         */
        virtual std::expected<Buffer, std::error_code> 
        create_vertex_buffer(std::span<const std::byte> data) = 0;

        /*!
         * @brief Create an index buffer.
         * @param data Index data to upload.
         * @return Buffer handle or error.
         */
        virtual std::expected<Buffer, std::error_code> 
        create_index_buffer(std::span<const std::byte> data) = 0;

        /*!
         * @brief Submit a simple draw call (for MVP).
         * @param vertex_buffer Vertex buffer to bind.
         * @param index_buffer Index buffer to bind.
         * @param index_count Number of indices to draw.
         * @return Success or error.
         */
        virtual std::expected<void, std::error_code>
        draw_indexed(const Buffer& vertex_buffer, 
                    const Buffer& index_buffer, 
                    uint32_t index_count) = 0;

        /*!
         * @brief Clear the current render target.
         */
        virtual void clear() = 0;

        /*!
         * @brief Present the rendered frame (swap buffers).
         */
        virtual void present() = 0;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_DEVICE_H
