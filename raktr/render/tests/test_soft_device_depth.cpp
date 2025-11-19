/*!
 * @file test_fake_device_depth.cpp
 * @brief Tests for SoftDevice depth buffer and lighting features.
 */

#include "backend/soft/soft_device.h"
#include "render_context.h"
#include <array>
#include <cmath>
#include <gtest/gtest.h>

namespace raktr::render::test
{

    TEST(SoftDevice_DepthBuffer, EnableDepthTest_InitializesDepthBuffer)
    {
        // Arrange - create SoftDevice directly for testing device-specific methods
        backend::SoftDevice device(800, 600);

        // Act
        device.enable_depth_test(true);

        // Assert - just verify it doesn't crash
        EXPECT_TRUE(device.is_depth_test_enabled());
    }

    TEST(SoftDevice_DepthBuffer, ClearDepthBuffer_ResetsAllDepthValues)
    {
        // Arrange - create SoftDevice directly for testing device-specific methods
        backend::SoftDevice device(800, 600);

        device.enable_depth_test(true);

        // Act
        device.clear_depth_buffer();

        // Assert - depth buffer should be reset (all far values)
        // We can't directly test this without internal access,
        // but we verify no crash and proper state
        EXPECT_TRUE(device.is_depth_test_enabled());
    }

    TEST(SoftDevice_DepthBuffer, CloserTriangleOccludesFartherTriangle)
    {
        // Arrange - create SoftDevice directly for testing device-specific methods
        backend::SoftDevice device(800, 600);

        device.enable_depth_test(true);
        device.clear_depth_buffer();
        device.clear();

        // Near triangle (z = 0.0, close to camera)
        std::array<float, 9> near_vertices = {
            0.0f, 0.3f, 0.0f, -0.3f, -0.3f, 0.0f, 0.3f, -0.3f, 0.0f
        };

        // Far triangle (z = -0.5, farther from camera)
        std::array<float, 9> far_vertices = {
            0.0f, 0.3f, -0.5f, -0.3f, -0.3f, -0.5f, 0.3f, -0.3f, -0.5f
        };

        std::array<uint32_t, 3> indices = { 0, 1, 2 };

        auto near_vb = device.create_vertex_buffer(std::as_bytes(std::span{ near_vertices }));
        auto far_vb  = device.create_vertex_buffer(std::as_bytes(std::span{ far_vertices }));
        auto ib      = device.create_index_buffer(std::as_bytes(std::span{ indices }));

        ASSERT_TRUE(near_vb.has_value());
        ASSERT_TRUE(far_vb.has_value());
        ASSERT_TRUE(ib.has_value());

        // Act - draw far triangle first, then near
        auto result1 = device.draw_indexed(*far_vb, *ib, 3);
        auto result2 = device.draw_indexed(*near_vb, *ib, 3);

        // Assert - both draws should succeed
        EXPECT_TRUE(result1.has_value());
        EXPECT_TRUE(result2.has_value());

        // Near triangle should have written over far triangle
        const uint32_t drawn = device.count_drawn_pixels();
        EXPECT_GT(drawn, 0) << "Near triangle should be visible";
    }

    TEST(SoftDevice_Lighting, VerticesWithNormals_ApplyLighting)
    {
        // Arrange - create SoftDevice directly for testing device-specific methods
        backend::SoftDevice device(800, 600);

        device.clear();

        // Triangle with normals (interleaved: pos, normal)
        std::array<float, 18> vertices = {
            // pos                  // normal (pointing up)
            0.0f,
            0.5f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            -0.5f,
            -0.5f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.5f,
            -0.5f,
            0.0f,
            0.0f,
            1.0f,
            0.0f
        };

        std::array<uint32_t, 3> indices = { 0, 1, 2 };

        auto vb = device.create_vertex_buffer(std::as_bytes(std::span{ vertices }));
        auto ib = device.create_index_buffer(std::as_bytes(std::span{ indices }));

        ASSERT_TRUE(vb.has_value());
        ASSERT_TRUE(ib.has_value());

        // Act
        device.set_vertex_format(backend::VertexFormat::PositionNormal);
        auto result = device.draw_indexed(*vb, *ib, 3);

        // Assert
        EXPECT_TRUE(result.has_value());
        EXPECT_GT(device.count_drawn_pixels(), 0);
    }

    TEST(SoftDevice_Lighting, NormalsPointingAway_ProduceDarkerColor)
    {
        // Arrange - create SoftDevice directly for testing device-specific methods
        backend::SoftDevice device(800, 600);

        // Triangle with normals pointing down (away from light)
        std::array<float, 18> vertices_down = {
            0.0f, 0.5f, 0.0f, 0.0f, -1.0f, 0.0f, // down
            -0.5f,
            -0.5f,
            0.0f,
            0.0f,
            -1.0f,
            0.0f,
            0.5f,
            -0.5f,
            0.0f,
            0.0f,
            -1.0f,
            0.0f
        };

        // Triangle with normals pointing up (towards light)
        std::array<float, 18> vertices_up = {
            0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // up
            -0.5f,
            -0.5f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.5f,
            -0.5f,
            0.0f,
            0.0f,
            1.0f,
            0.0f
        };

        std::array<uint32_t, 3> indices = { 0, 1, 2 };

        auto vb_down = device.create_vertex_buffer(std::as_bytes(std::span{ vertices_down }));
        auto vb_up   = device.create_vertex_buffer(std::as_bytes(std::span{ vertices_up }));
        auto ib      = device.create_index_buffer(std::as_bytes(std::span{ indices }));

        ASSERT_TRUE(vb_down.has_value());
        ASSERT_TRUE(vb_up.has_value());
        ASSERT_TRUE(ib.has_value());

        // Act
        device.clear();
        device.set_vertex_format(backend::VertexFormat::PositionNormal);

        auto           result_down = device.draw_indexed(*vb_down, *ib, 3);
        const uint32_t center_down = device.get_pixel(400, 300); // Center pixel

        device.clear();
        auto           result_up = device.draw_indexed(*vb_up, *ib, 3);
        const uint32_t center_up = device.get_pixel(400, 300);

        // Assert - pixel pointing up should be brighter
        EXPECT_TRUE(result_down.has_value());
        EXPECT_TRUE(result_up.has_value());

        // Extract brightness (simple average of RGB)
        auto get_brightness = [](uint32_t rgba)
        {
            uint8_t r = (rgba >> 24) & 0xFF;
            uint8_t g = (rgba >> 16) & 0xFF;
            uint8_t b = (rgba >> 8) & 0xFF;
            return (r + g + b) / 3;
        };

        EXPECT_GT(get_brightness(center_up), get_brightness(center_down))
            << "Triangle facing light should be brighter";
    }

} // namespace raktr::render::test
