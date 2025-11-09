/*!
 * @file fake_device.cpp
 * @brief Fake device implementation - software renderer for testing.
 */

#include "fake_device.h"
#include "render_error.h"
#include <algorithm>
#include <cmath>
#include <memory>

namespace raktr::render::backend
{
    FakeDevice::FakeDevice(uint32_t width, uint32_t height)
        : _framebuffer(width, height)
    {
    }

    std::expected<Buffer, std::error_code>
    FakeDevice::create_vertex_buffer(std::span<const std::byte> data)
    {
        if (data.empty())
        {
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
        if (data.empty())
        {
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
                             uint32_t      index_count)
    {
        if (!vertex_buffer.is_valid())
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }
        if (!index_buffer.is_valid())
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }
        if (index_count == 0)
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        // Verify buffers exist in storage
        auto vb_it = _buffers.find(vertex_buffer.id());
        auto ib_it = _buffers.find(index_buffer.id());

        if (vb_it == _buffers.end() || ib_it == _buffers.end())
        {
            return std::unexpected(make_error_code(RenderError::InvalidOperation));
        }

        const auto& vb_data = vb_it->second.data;
        const auto& ib_data = ib_it->second.data;

        const uint32_t* indices  = reinterpret_cast<const uint32_t*>(ib_data.data());
        const float*    vertices = reinterpret_cast<const float*>(vb_data.data());

        // Determine vertex stride based on format
        uint32_t vertex_stride = 3; // default: position only
        switch (_vertex_format)
        {
            case VertexFormat::Position:
                vertex_stride = 3;
                break;
            case VertexFormat::PositionNormal:
                vertex_stride = 6;
                break;
            case VertexFormat::PositionUV:
                vertex_stride = 5;
                break;
            case VertexFormat::PositionNormalUV:
                vertex_stride = 8;
                break;
        }

        // Rasterize triangles
        for (uint32_t i = 0; i < index_count; i += 3)
        {
            const uint32_t i0 = indices[i];
            const uint32_t i1 = indices[i + 1];
            const uint32_t i2 = indices[i + 2];

            const float* v0 = &vertices[i0 * vertex_stride];
            const float* v1 = &vertices[i1 * vertex_stride];
            const float* v2 = &vertices[i2 * vertex_stride];

            if (_vertex_format == VertexFormat::PositionNormal ||
                _vertex_format == VertexFormat::PositionNormalUV)
            {
                rasterize_triangle_with_normals(v0, v1, v2);
            }
            else
            {
                rasterize_triangle(v0, v1, v2);
            }
        }

        return {};
    }

    void FakeDevice::clear()
    {
        std::fill(_framebuffer.pixels.begin(), _framebuffer.pixels.end(), _framebuffer.clear_color);
        if (_depth_test_enabled)
        {
            std::fill(_framebuffer.depth.begin(), _framebuffer.depth.end(), 1.0f);
        }
    }

    void FakeDevice::present()
    {
        // No-op for fake device (already in framebuffer)
    }

    uint32_t FakeDevice::get_pixel(uint32_t x, uint32_t y) const
    {
        if (x >= _framebuffer.width || y >= _framebuffer.height)
        {
            return 0;
        }
        return _framebuffer.pixels[y * _framebuffer.width + x];
    }

    uint32_t FakeDevice::count_drawn_pixels() const
    {
        uint32_t count = 0;
        for (const auto& pixel : _framebuffer.pixels)
        {
            if (pixel != _framebuffer.clear_color)
            {
                ++count;
            }
        }
        return count;
    }

    void FakeDevice::set_clear_color(uint32_t color)
    {
        _framebuffer.clear_color = color;
    }

    void FakeDevice::enable_depth_test(bool enabled)
    {
        _depth_test_enabled = enabled;
        if (enabled)
        {
            clear_depth_buffer();
        }
    }

    bool FakeDevice::is_depth_test_enabled() const
    {
        return _depth_test_enabled;
    }

    void FakeDevice::clear_depth_buffer()
    {
        std::fill(_framebuffer.depth.begin(), _framebuffer.depth.end(), 1.0f);
    }

    void FakeDevice::set_vertex_format(VertexFormat format)
    {
        _vertex_format = format;
    }

    void FakeDevice::rasterize_triangle(const float* v0, const float* v1, const float* v2)
    {
        // Convert NDC [-1,1] to screen space [0, width/height]
        const int   x0 = static_cast<int>((v0[0] + 1.0f) * 0.5f * _framebuffer.width);
        const int   y0 = static_cast<int>((1.0f - v0[1]) * 0.5f * _framebuffer.height);
        const float z0 = v0[2];

        const int   x1 = static_cast<int>((v1[0] + 1.0f) * 0.5f * _framebuffer.width);
        const int   y1 = static_cast<int>((1.0f - v1[1]) * 0.5f * _framebuffer.height);
        const float z1 = v1[2];

        const int   x2 = static_cast<int>((v2[0] + 1.0f) * 0.5f * _framebuffer.width);
        const int   y2 = static_cast<int>((1.0f - v2[1]) * 0.5f * _framebuffer.height);
        const float z2 = v2[2];

        // Bounding box
        const int min_x = std::max(0, std::min({ x0, x1, x2 }));
        const int max_x = std::min(static_cast<int>(_framebuffer.width) - 1, std::max({ x0, x1, x2 }));
        const int min_y = std::max(0, std::min({ y0, y1, y2 }));
        const int max_y = std::min(static_cast<int>(_framebuffer.height) - 1, std::max({ y0, y1, y2 }));

        // Scanline rasterization
        constexpr uint32_t white = 0xFFFFFFFF;
        for (int y = min_y; y <= max_y; ++y)
        {
            for (int x = min_x; x <= max_x; ++x)
            {
                if (point_in_triangle(x, y, x0, y0, x1, y1, x2, y2))
                {
                    const uint32_t pixel_idx = y * _framebuffer.width + x;

                    // Simple depth interpolation (use center z)
                    const float z = (z0 + z1 + z2) / 3.0f;

                    // Depth test
                    if (_depth_test_enabled)
                    {
                        if (z < _framebuffer.depth[pixel_idx])
                        {
                            _framebuffer.depth[pixel_idx]  = z;
                            _framebuffer.pixels[pixel_idx] = white;
                        }
                    }
                    else
                    {
                        _framebuffer.pixels[pixel_idx] = white;
                    }
                }
            }
        }
    }

    void FakeDevice::rasterize_triangle_with_normals(const float* v0, const float* v1, const float* v2)
    {
        // Extract positions and normals
        // Format: x, y, z, nx, ny, nz
        const int   x0 = static_cast<int>((v0[0] + 1.0f) * 0.5f * _framebuffer.width);
        const int   y0 = static_cast<int>((1.0f - v0[1]) * 0.5f * _framebuffer.height);
        const float z0 = v0[2];

        const int   x1 = static_cast<int>((v1[0] + 1.0f) * 0.5f * _framebuffer.width);
        const int   y1 = static_cast<int>((1.0f - v1[1]) * 0.5f * _framebuffer.height);
        const float z1 = v1[2];

        const int   x2 = static_cast<int>((v2[0] + 1.0f) * 0.5f * _framebuffer.width);
        const int   y2 = static_cast<int>((1.0f - v2[1]) * 0.5f * _framebuffer.height);
        const float z2 = v2[2];

        // Average normal for flat shading
        const float normal[3] = {
            (v0[3] + v1[3] + v2[3]) / 3.0f,
            (v0[4] + v1[4] + v2[4]) / 3.0f,
            (v0[5] + v1[5] + v2[5]) / 3.0f
        };

        // Calculate lighting
        const float lighting = calculate_lighting(normal);

        // Bounding box
        const int min_x = std::max(0, std::min({ x0, x1, x2 }));
        const int max_x = std::min(static_cast<int>(_framebuffer.width) - 1, std::max({ x0, x1, x2 }));
        const int min_y = std::max(0, std::min({ y0, y1, y2 }));
        const int max_y = std::min(static_cast<int>(_framebuffer.height) - 1, std::max({ y0, y1, y2 }));

        // Apply lighting to base color
        constexpr uint32_t base_color = 0xFFFFFFFF;
        const uint32_t     lit_color  = apply_lighting_to_color(base_color, lighting);

        // Scanline rasterization
        for (int y = min_y; y <= max_y; ++y)
        {
            for (int x = min_x; x <= max_x; ++x)
            {
                if (point_in_triangle(x, y, x0, y0, x1, y1, x2, y2))
                {
                    const uint32_t pixel_idx = y * _framebuffer.width + x;

                    // Simple depth interpolation
                    const float z = (z0 + z1 + z2) / 3.0f;

                    // Depth test
                    if (_depth_test_enabled)
                    {
                        if (z < _framebuffer.depth[pixel_idx])
                        {
                            _framebuffer.depth[pixel_idx]  = z;
                            _framebuffer.pixels[pixel_idx] = lit_color;
                        }
                    }
                    else
                    {
                        _framebuffer.pixels[pixel_idx] = lit_color;
                    }
                }
            }
        }
    }

    bool FakeDevice::point_in_triangle(int px, int py, int x0, int y0, int x1, int y1, int x2, int y2) const
    {
        // Edge function test (cross product sign)
        auto sign = [](int px, int py, int ax, int ay, int bx, int by) -> int
        {
            return (px - bx) * (ay - by) - (ax - bx) * (py - by);
        };

        const int d0 = sign(px, py, x0, y0, x1, y1);
        const int d1 = sign(px, py, x1, y1, x2, y2);
        const int d2 = sign(px, py, x2, y2, x0, y0);

        const bool has_neg = (d0 < 0) || (d1 < 0) || (d2 < 0);
        const bool has_pos = (d0 > 0) || (d1 > 0) || (d2 > 0);

        return !(has_neg && has_pos);
    }

    float FakeDevice::calculate_lighting(const float* normal) const
    {
        // Simple directional light from above
        constexpr float light_dir[3] = { 0.0f, 1.0f, 0.0f };
        constexpr float ambient      = 0.3f;

        // Normalize normal (in case it's not unit length)
        const float len = std::sqrt(normal[0] * normal[0] +
                                    normal[1] * normal[1] +
                                    normal[2] * normal[2]);
        if (len < 0.001f)
            return ambient;

        const float nx = normal[0] / len;
        const float ny = normal[1] / len;
        const float nz = normal[2] / len;

        // Dot product with light direction
        const float dot = std::max(0.0f, nx * light_dir[0] + ny * light_dir[1] + nz * light_dir[2]);

        return ambient + (1.0f - ambient) * dot;
    }

    uint32_t FakeDevice::apply_lighting_to_color(uint32_t base_color, float intensity) const
    {
        // Extract RGBA components
        const uint8_t r = static_cast<uint8_t>(((base_color >> 24) & 0xFF) * intensity);
        const uint8_t g = static_cast<uint8_t>(((base_color >> 16) & 0xFF) * intensity);
        const uint8_t b = static_cast<uint8_t>(((base_color >> 8) & 0xFF) * intensity);
        const uint8_t a = (base_color & 0xFF);

        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    // Stub implementations for unsupported Device interface methods
    std::expected<Buffer, std::error_code>
    FakeDevice::create_uniform_buffer(size_t)
    {
        return std::unexpected(make_error_code(std::errc::not_supported));
    }

    std::expected<void, std::error_code>
    FakeDevice::update_uniform_buffer(const Buffer&, std::span<const std::byte>)
    {
        return std::unexpected(make_error_code(std::errc::not_supported));
    }

    void FakeDevice::set_uniform_buffer(const Buffer&)
    {
        // No-op
    }

    std::expected<void, std::error_code>
    FakeDevice::resize(uint32_t, uint32_t)
    {
        return {}; // Success, but do nothing
    }

    void FakeDevice::set_aspect_ratio(AspectRatio, float)
    {
        // No-op
    }

    AspectRatio FakeDevice::aspect_ratio() const
    {
        return AspectRatio::Ratio_16_9;
    }

    const Viewport& FakeDevice::viewport() const
    {
        static Viewport default_vp{};
        return default_vp;
    }

} // namespace raktr::render::backend
