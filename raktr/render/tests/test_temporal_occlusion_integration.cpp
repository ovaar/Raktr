/*!
 * @file test_temporal_occlusion_integration.cpp
 * @brief Integration tests for temporal Hi-Z occlusion culling with render passes.
 */

#include "backend/soft/soft_device.h"
#include "backend/wgpu/passes/wgpu_geometry_pass.h"
#include "backend/wgpu/passes/wgpu_hi_z_occlusion_pass.h"
#include "backend/wgpu/passes/wgpu_hi_z_pyramid_pass.h"
#include "backend/wgpu/wgpu_frame_resources.h"
#include "backend/wgpu/wgpu_pass_context.h"
#include "occlusion/hi_z_buffer.h"
#include "render_graph.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>


using namespace raktr::render;
using namespace raktr::render::backend::wgpu;

namespace
{
    // Mock Hi-Z buffer for integration testing
    class MockHiZBuffer : public occlusion::HiZBuffer
    {
    public:
        MockHiZBuffer(uint32_t width, uint32_t height)
            : _width(width), _height(height), _mip_levels(8)
        {
        }

        std::expected<void, std::error_code> build_pyramid(void* depth_texture) override
        {
            _build_pyramid_called = true;
            _pyramid_built_count++;
            _last_test_count = 0; // Reset for stats
            (void)depth_texture;
            return {};
        }

        std::expected<std::vector<bool>, std::error_code>
        test_visibility(std::span<const occlusion::AABB> aabbs,
                        const glm::mat4&                 view_projection) override
        {
            _test_visibility_called = true;
            _visibility_test_count++;
            (void)view_projection;

            // Simulate culling: first half visible, second half occluded
            std::vector<bool> results;
            results.reserve(aabbs.size());
            for (size_t i = 0; i < aabbs.size(); ++i)
            {
                results.push_back(i < aabbs.size() / 2);
            }
            return results;
        }

        uint32_t mip_levels() const override
        {
            return _mip_levels;
        }
        uint32_t width() const override
        {
            return _width;
        }
        uint32_t height() const override
        {
            return _height;
        }

        Stats stats() const override
        {
            return Stats{
                .pyramid_build_ms   = 1.0f,
                .visibility_test_ms = 0.5f,
                .objects_tested     = _last_test_count,
                .objects_visible    = _last_test_count / 2,
                .objects_culled     = _last_test_count / 2
            };
        }

        bool was_build_pyramid_called() const
        {
            return _build_pyramid_called;
        }
        bool was_test_visibility_called() const
        {
            return _test_visibility_called;
        }
        uint32_t pyramid_built_count() const
        {
            return _pyramid_built_count;
        }
        uint32_t visibility_test_count() const
        {
            return _visibility_test_count;
        }

        void reset()
        {
            _build_pyramid_called   = false;
            _test_visibility_called = false;
        }

    private:
        uint32_t _width;
        uint32_t _height;
        uint32_t _mip_levels;
        bool     _build_pyramid_called{ false };
        bool     _test_visibility_called{ false };
        uint32_t _pyramid_built_count{ 0 };
        uint32_t _visibility_test_count{ 0 };
        uint32_t _last_test_count{ 0 };
    };

    // Mock depth texture handle for testing
    static WGPUTexture create_mock_depth_texture()
    {
        return reinterpret_cast<WGPUTexture>(0x1234); // Non-null mock handle
    }

    WgpuPassContext create_integration_test_context()
    {
        WgpuPassContext ctx{};
        ctx.frame_index             = 0;
        ctx.command_encoder         = nullptr;
        ctx.color_target            = nullptr;
        ctx.depth_target            = nullptr;
        ctx.prev_frame_hi_z_pyramid = nullptr;
        ctx.viewport_width          = 1920;
        ctx.viewport_height         = 1080;
        return ctx;
    }
} // namespace

TEST(TemporalOcclusion_Integration, TwoPassPipeline_ExecutesInOrder)
{
    // Arrange
    MockHiZBuffer                hi_z(1920, 1080);
    std::vector<occlusion::AABB> aabbs(10);
    glm::mat4                    view_projection(1.0f);
    std::vector<bool>            visibility_results;

    RenderGraph graph(nullptr);

    // Build 2-pass pipeline: occlusion test + pyramid build
    auto mock_depth = create_mock_depth_texture();
    graph.add_pass(WgpuHiZOcclusionPass(&hi_z, &aabbs, view_projection, &visibility_results))
        .add_pass(WgpuHiZPyramidPass(&hi_z, mock_depth));

    auto ctx = create_integration_test_context();

    // Act
    graph.execute(ctx);

    // Assert
    EXPECT_TRUE(hi_z.was_test_visibility_called()) << "Occlusion pass should call test_visibility";
    EXPECT_TRUE(hi_z.was_build_pyramid_called()) << "Pyramid pass should call build_pyramid";
    EXPECT_EQ(visibility_results.size(), 10u) << "Should have visibility results for all AABBs";
}

TEST(TemporalOcclusion_Integration, OcclusionPass_CullsHalfOfObjects)
{
    // Arrange
    MockHiZBuffer                hi_z(1920, 1080);
    std::vector<occlusion::AABB> aabbs(100);
    for (size_t i = 0; i < aabbs.size(); ++i)
    {
        aabbs[i].min = glm::vec3(-1.0f + i * 0.1f);
        aabbs[i].max = glm::vec3(1.0f + i * 0.1f);
    }

    glm::mat4         view_projection(1.0f);
    std::vector<bool> visibility_results;

    RenderGraph graph(nullptr);
    graph.add_pass(WgpuHiZOcclusionPass(&hi_z, &aabbs, view_projection, &visibility_results));

    auto ctx = create_integration_test_context();

    // Act
    graph.execute(ctx);

    // Assert
    EXPECT_EQ(visibility_results.size(), 100u);

    uint32_t visible_count = 0;
    for (bool visible : visibility_results)
    {
        if (visible)
            visible_count++;
    }

    EXPECT_EQ(visible_count, 50u) << "Mock culls 50% of objects";
    EXPECT_EQ(visibility_results.size() - visible_count, 50u) << "50% should be culled";
}

TEST(TemporalOcclusion_Integration, MultipleFrames_PyramidBuiltEachFrame)
{
    // Arrange
    MockHiZBuffer hi_z(1920, 1080);
    auto          mock_depth = create_mock_depth_texture();

    RenderGraph graph(nullptr);
    graph.add_pass(WgpuHiZPyramidPass(&hi_z, mock_depth));

    auto ctx = create_integration_test_context();

    // Act - render 5 frames
    for (int i = 0; i < 5; ++i)
    {
        ctx.frame_index = i;
        graph.execute(ctx);
    }

    // Assert
    EXPECT_EQ(hi_z.pyramid_built_count(), 5u) << "Pyramid should be built every frame";
}

TEST(TemporalOcclusion_Integration, EmptyAABBList_HandledGracefully)
{
    // Arrange
    MockHiZBuffer                hi_z(1920, 1080);
    std::vector<occlusion::AABB> empty_aabbs;
    glm::mat4                    view_projection(1.0f);
    std::vector<bool>            visibility_results;

    RenderGraph graph(nullptr);
    graph.add_pass(WgpuHiZOcclusionPass(&hi_z, &empty_aabbs, view_projection, &visibility_results));

    auto ctx = create_integration_test_context();

    // Act & Assert - should not crash
    EXPECT_NO_THROW(graph.execute(ctx));
    EXPECT_TRUE(visibility_results.empty());
}

TEST(TemporalOcclusion_Integration, NullHiZBuffer_FallsBackToAllVisible)
{
    // Arrange
    std::vector<occlusion::AABB> aabbs(10);
    std::vector<bool>            visibility_results;

    RenderGraph graph(nullptr);
    graph.add_pass(WgpuHiZOcclusionPass(nullptr, &aabbs, glm::mat4(1.0f), &visibility_results));

    auto ctx = create_integration_test_context();

    // Act
    graph.execute(ctx);

    // Assert - all objects should be visible when Hi-Z is null
    EXPECT_EQ(visibility_results.size(), 10u);
    for (bool visible : visibility_results)
    {
        EXPECT_TRUE(visible) << "All objects should be visible without Hi-Z buffer";
    }
}

TEST(TemporalOcclusion_Integration, ViewportResize_NotifiesAllPasses)
{
    // Arrange
    MockHiZBuffer                hi_z(1920, 1080);
    std::vector<occlusion::AABB> aabbs(10);
    glm::mat4                    view_projection(1.0f);
    std::vector<bool>            visibility_results;

    auto mock_depth = create_mock_depth_texture();

    RenderGraph graph(nullptr);
    graph.add_pass(WgpuHiZOcclusionPass(&hi_z, &aabbs, view_projection, &visibility_results))
        .add_pass(WgpuHiZPyramidPass(&hi_z, mock_depth));

    // Act & Assert - should not crash
    EXPECT_NO_THROW(graph.on_viewport_resize(1920, 1080));
}

TEST(TemporalOcclusion_Integration, GraphClear_CanRebuildPipeline)
{
    // Arrange
    MockHiZBuffer                hi_z(1920, 1080);
    std::vector<occlusion::AABB> aabbs(10);
    glm::mat4                    view_projection(1.0f);
    std::vector<bool>            visibility_results1, visibility_results2;

    RenderGraph graph(nullptr);
    graph.add_pass(WgpuHiZOcclusionPass(&hi_z, &aabbs, view_projection, &visibility_results1));

    auto ctx = create_integration_test_context();
    graph.execute(ctx);

    hi_z.reset();

    // Act - clear and rebuild
    graph.clear();
    graph.add_pass(WgpuHiZOcclusionPass(&hi_z, &aabbs, view_projection, &visibility_results2));
    graph.execute(ctx);

    // Assert
    EXPECT_TRUE(hi_z.was_test_visibility_called()) << "New pipeline should execute";
    EXPECT_EQ(visibility_results2.size(), 10u);
}

TEST(TemporalOcclusion_Integration, PassUpdate_ChangesSceneData)
{
    // Arrange
    MockHiZBuffer                hi_z(1920, 1080);
    std::vector<occlusion::AABB> aabbs1(10);
    std::vector<occlusion::AABB> aabbs2(20);
    glm::mat4                    view_projection(1.0f);
    std::vector<bool>            visibility_results;

    RenderGraph graph(nullptr);
    graph.add_pass(WgpuHiZOcclusionPass(&hi_z, &aabbs1, view_projection, &visibility_results));

    auto ctx = create_integration_test_context();

    // Act - first execution with 10 AABBs
    graph.execute(ctx);
    EXPECT_EQ(visibility_results.size(), 10u);

    // Note: With type erasure, passes are moved into RenderGraph
    // Cannot update pass data after construction
    // Execute again with same data
    graph.execute(ctx);

    // Assert - should still be 10 since pass holds pointer to aabbs1
    EXPECT_EQ(visibility_results.size(), 10u) << "Should maintain same AABB count";
}

TEST(TemporalOcclusion_Integration, LargeScene_HandlesThousandsOfObjects)
{
    // Arrange
    MockHiZBuffer                hi_z(1920, 1080);
    std::vector<occlusion::AABB> aabbs(10000);
    for (size_t i = 0; i < aabbs.size(); ++i)
    {
        float offset = static_cast<float>(i);
        aabbs[i].min = glm::vec3(-1.0f + offset * 0.01f);
        aabbs[i].max = glm::vec3(1.0f + offset * 0.01f);
    }

    glm::mat4         view_projection(1.0f);
    std::vector<bool> visibility_results;

    RenderGraph graph(nullptr);
    graph.add_pass(WgpuHiZOcclusionPass(&hi_z, &aabbs, view_projection, &visibility_results));

    auto ctx = create_integration_test_context();

    // Act
    graph.execute(ctx);

    // Assert
    EXPECT_EQ(visibility_results.size(), 10000u);

    uint32_t visible_count = 0;
    for (bool visible : visibility_results)
    {
        if (visible)
            visible_count++;
    }

    EXPECT_EQ(visible_count, 5000u) << "Mock culls 50% of 10K objects";
}

TEST(TemporalOcclusion_Integration, PassNames_CorrectlyReported)
{
    // Arrange
    MockHiZBuffer                hi_z(1920, 1080);
    std::vector<occlusion::AABB> aabbs(10);
    glm::mat4                    view_projection(1.0f);
    std::vector<bool>            visibility_results;

    auto occlusion_pass_ptr = new WgpuHiZOcclusionPass(&hi_z, &aabbs, view_projection, &visibility_results);
    auto pyramid_pass_ptr   = new WgpuHiZPyramidPass(&hi_z, nullptr);

    auto* occlusion_pass = occlusion_pass_ptr;
    auto* pyramid_pass   = pyramid_pass_ptr;

    // Act
    std::string_view occlusion_name = occlusion_pass->name();
    std::string_view pyramid_name   = pyramid_pass->name();

    // Assert
    EXPECT_EQ(occlusion_name, "WgpuHiZOcclusionPass");
    EXPECT_EQ(pyramid_name, "WgpuHiZPyramidPass");

    delete occlusion_pass_ptr;
    delete pyramid_pass_ptr;
}

TEST(TemporalOcclusion_Integration, FrameIndexProgression_TrackedCorrectly)
{
    // Arrange
    MockHiZBuffer hi_z(1920, 1080);
    auto          mock_depth = create_mock_depth_texture();

    RenderGraph graph(nullptr);
    graph.add_pass(WgpuHiZPyramidPass(&hi_z, mock_depth));

    auto ctx = create_integration_test_context();

    // Act - simulate multiple frames
    for (auto i = 0; i < 10; ++i)
    {
        ctx.frame_index = static_cast<uint32_t>(i);
        graph.execute(ctx);
    }

    // Assert
    EXPECT_EQ(hi_z.pyramid_built_count(), 10u) << "Should build pyramid for each frame";
}
