/*!
 * @file fake_device.cpp
 * @brief Fake device implementation - software renderer for testing.
 */

#include "fake_device.h"
#include "render_error.h"
#include <memory>
#include <algorithm>

namespace raktr::render::backend
{
    FakeDevice::FakeDevice(uint32_t width, uint32_t height)
        : _framebuffer(width, height)
    {
    }

    std::expected<Buffer, std::error_code> 
    FakeDevice::create_vertex_buffer(std::span<const std::byte> data)
    {
        if (data.empty()) {
            return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
        }

        const uint64_t id = _next_buffer_id++;
        
        // Store the actual buffer data
        _buffers[id] = BufferData{
            .data = std::vector<std::byte>(data.begin(), data.end()),
            .type = BufferType::Vertex
        };

        return Buffer(id, BufferType::Vertex);
    }

    std::expected<Buffer, std::error_code> 
    FakeDevice::create_index_buffer(std::span<const std::byte> data)
    {
        if (data.empty()) {
            return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
        }

        const uint64_t id = _next_buffer_id++;
        
        // Store the actual buffer data
        _buffers[id] = BufferData{
            .data = std::vector<std::byte>(data.begin(), data.end()),
            .type = BufferType::Index
        };

        return Buffer(id, BufferType::Index);
    }

    std::expected<void, std::error_code>
    FakeDevice::draw_indexed(const Buffer& vertex_buffer, 
                            const Buffer& index_buffer, 
                            uint32_t index_count)
    {
        if (!vertex_buffer.is_valid()) {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }
        if (!index_buffer.is_valid()) {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }
        if (index_count == 0) {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Verify buffers exist in storage
        auto vb_it = _buffers.find(vertex_buffer.id());
        auto ib_it = _buffers.find(index_buffer.id());
        
        if (vb_it == _buffers.end() || ib_it == _buffers.end()) {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Basic software rasterization - assumes vec3 positions (12 bytes/vertex)
        const auto& vb_data = vb_it->second.data;
        const auto& ib_data = ib_it->second.data;
        
        const uint32_t* indices = reinterpret_cast<const uint32_t*>(ib_data.data());
        const float* vertices = reinterpret_cast<const float*>(vb_data.data());

        // Rasterize triangles
        for (uint32_t i = 0; i < index_count; i += 3) {
            const uint32_t i0 = indices[i];
            const uint32_t i1 = indices[i + 1];
            const uint32_t i2 = indices[i + 2];

            const float* v0 = &vertices[i0 * 3];
            const float* v1 = &vertices[i1 * 3];
            const float* v2 = &vertices[i2 * 3];

            rasterize_triangle(v0, v1, v2);
        }

        return {};
    }

    void FakeDevice::clear()
    {
        std::fill(_framebuffer.pixels.begin(), _framebuffer.pixels.end(), 
                 _framebuffer.clear_color);
    }

    void FakeDevice::present()
    {
        // No-op for fake device (already in framebuffer)
    }

    uint32_t FakeDevice::get_pixel(uint32_t x, uint32_t y) const
    {
        if (x >= _framebuffer.width || y >= _framebuffer.height) {
            return 0;
        }
        return _framebuffer.pixels[y * _framebuffer.width + x];
    }

    uint32_t FakeDevice::count_drawn_pixels() const
    {
        uint32_t count = 0;
        for (const auto& pixel : _framebuffer.pixels) {
            if (pixel != _framebuffer.clear_color) {
                ++count;
            }
        }
        return count;
    }

    void FakeDevice::set_clear_color(uint32_t color)
    {
        _framebuffer.clear_color = color;
    }

    void FakeDevice::rasterize_triangle(const float* v0, const float* v1, const float* v2)
    {
        // Convert NDC [-1,1] to screen space [0, width/height]
        const int x0 = static_cast<int>((v0[0] + 1.0f) * 0.5f * _framebuffer.width);
        const int y0 = static_cast<int>((1.0f - v0[1]) * 0.5f * _framebuffer.height);
        const int x1 = static_cast<int>((v1[0] + 1.0f) * 0.5f * _framebuffer.width);
        const int y1 = static_cast<int>((1.0f - v1[1]) * 0.5f * _framebuffer.height);
        const int x2 = static_cast<int>((v2[0] + 1.0f) * 0.5f * _framebuffer.width);
        const int y2 = static_cast<int>((1.0f - v2[1]) * 0.5f * _framebuffer.height);

        // Bounding box
        const int min_x = std::max(0, std::min({x0, x1, x2}));
        const int max_x = std::min(static_cast<int>(_framebuffer.width) - 1, std::max({x0, x1, x2}));
        const int min_y = std::max(0, std::min({y0, y1, y2}));
        const int max_y = std::min(static_cast<int>(_framebuffer.height) - 1, std::max({y0, y1, y2}));

        // Scanline rasterization
        constexpr uint32_t white = 0xFFFFFFFF;
        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                if (point_in_triangle(x, y, x0, y0, x1, y1, x2, y2)) {
                    _framebuffer.pixels[y * _framebuffer.width + x] = white;
                }
            }
        }
    }

    bool FakeDevice::point_in_triangle(int px, int py, int x0, int y0, int x1, int y1, int x2, int y2) const
    {
        // Edge function test (cross product sign)
        auto sign = [](int px, int py, int ax, int ay, int bx, int by) -> int {
            return (px - bx) * (ay - by) - (ax - bx) * (py - by);
        };

        const int d0 = sign(px, py, x0, y0, x1, y1);
        const int d1 = sign(px, py, x1, y1, x2, y2);
        const int d2 = sign(px, py, x2, y2, x0, y0);

        const bool has_neg = (d0 < 0) || (d1 < 0) || (d2 < 0);
        const bool has_pos = (d0 > 0) || (d1 > 0) || (d2 > 0);

        return !(has_neg && has_pos);
    }

} // namespace raktr::render::backend
