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
     * @brief Vertex format specification for interleaved vertex data.
     */
    enum class VertexFormat
    {
        Position,          // 3 floats: x, y, z
        PositionNormal,    // 6 floats: x, y, z, nx, ny, nz
        PositionUV,        // 5 floats: x, y, z, u, v
        PositionNormalUV   // 8 floats: x, y, z, nx, ny, nz, u, v
    };

    /*!
     * @brief Fake device for testing - implements real software rendering.
     * 
     * Unlike a mock, this actually stores buffer data and renders to an
     * in-memory framebuffer, enabling true end-to-end rendering tests.
     * 
     * Features:
     * - Software rasterization with depth testing
     * - Simple directional lighting (ambient + diffuse)
     * - Normals and UV coordinate support
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

        /*!
         * @brief Enable or disable depth testing.
         */
        void enable_depth_test(bool enabled);

        /*!
         * @brief Check if depth testing is enabled.
         */
        bool is_depth_test_enabled() const;

        /*!
         * @brief Clear the depth buffer to far plane.
         */
        void clear_depth_buffer();

        /*!
         * @brief Set the vertex format for correct attribute parsing.
         */
        void set_vertex_format(VertexFormat format);

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
            std::vector<float> depth;      // Depth buffer (0.0 = near, 1.0 = far)
            uint32_t clear_color = 0x000000FF;  // Black, opaque

            Framebuffer(uint32_t w, uint32_t h) 
                : width(w), height(h), 
                  pixels(w * h, clear_color),
                  depth(w * h, 1.0f) {}
        };
        Framebuffer _framebuffer;

        // Render state
        bool _depth_test_enabled = false;
        VertexFormat _vertex_format = VertexFormat::Position;

        // Helper methods
        void rasterize_triangle(const float* v0, const float* v1, const float* v2);
        void rasterize_triangle_with_normals(const float* v0, const float* v1, const float* v2);
        bool point_in_triangle(int px, int py, int x0, int y0, int x1, int y1, int x2, int y2) const;
        float calculate_lighting(const float* normal) const;
        uint32_t apply_lighting_to_color(uint32_t base_color, float intensity) const;
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_BACKEND_FAKE_DEVICE_H
