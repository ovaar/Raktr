/*!
 * @file fake_device.h
 * @brief Fake device implementation - software renderer for testing.
 */

#ifndef RAKTR_RENDER_BACKEND_FAKE_DEVICE_H
#define RAKTR_RENDER_BACKEND_FAKE_DEVICE_H

#include "device.h"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace raktr::render::backend
{
    /*!
     * @brief Fake device for testing - implements real software rendering.
     * 
     * Unlike a mock, this actually stores buffer data and renders to an
     * in-memory framebuffer, enabling true end-to-end rendering tests.
     */
    class FakeDevice : public Device
    {
    public:
        FakeDevice(uint32_t width = 800, uint32_t height = 600);
        ~FakeDevice() override = default;

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

        /*!
         * @brief Get pixel color at (x, y) for testing.
         * @return RGBA8 color value, or 0 if out of bounds.
         */
        uint32_t get_pixel(uint32_t x, uint32_t y) const;

        /*!
         * @brief Count non-background pixels (for coverage tests).
         */
        uint32_t count_drawn_pixels() const;

        /*!
         * @brief Set clear color (RGBA8 format).
         */
        void set_clear_color(uint32_t color);

    private:
        // Buffer storage
        struct BufferData {
            std::vector<std::byte> data;
            BufferType type;
        };
        std::unordered_map<uint64_t, BufferData> _buffers;
        uint64_t _next_buffer_id = 1;

        // Framebuffer
        struct Framebuffer {
            uint32_t width;
            uint32_t height;
            std::vector<uint32_t> pixels;  // RGBA8 format
            uint32_t clear_color = 0x000000FF;  // Black, opaque

            Framebuffer(uint32_t w, uint32_t h) 
                : width(w), height(h), pixels(w * h, clear_color) {}
        };
        Framebuffer _framebuffer;

        // Helper methods
        void rasterize_triangle(const float* v0, const float* v1, const float* v2);
        bool point_in_triangle(int px, int py, int x0, int y0, int x1, int y1, int x2, int y2) const;
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_BACKEND_FAKE_DEVICE_H
