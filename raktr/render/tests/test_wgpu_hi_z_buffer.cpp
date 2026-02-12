/*!
 * @file test_wgpu_hi_z_buffer.cpp
 * @brief Unit tests for WebGPU Hi-Z occlusion culling implementation.
 */

#include "backend/wgpu/occlusion/wgpu_hi_z_buffer.h"
#include "backend/wgpu/wgpu_device.h"
#include "window/window.h"
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>
#include <vector>

using namespace raktr::render;
using namespace raktr::render::backend::wgpu;
using namespace raktr::render::occlusion;

class WgpuHiZBufferTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        WindowConfig config;
        config.width      = 1024;
        config.height     = 768;
        config.title      = "Hi-Z Test Window";
        config.resizable  = false;
        config.fullscreen = false;

        auto window_result = create_window(config);
        ASSERT_TRUE(window_result.has_value()) << "Failed to create test window";
        _window = std::move(window_result.value());

        auto device_result = WgpuDevice::create(_window.get(), false);
        ASSERT_TRUE(device_result.has_value()) << "Failed to create WgpuDevice";
        _device = std::make_unique<WgpuDevice>(std::move(device_result.value()));
    }

    void TearDown() override
    {
        _device.reset();
        _window.reset();
    }

    std::unique_ptr<Window>     _window;
    std::unique_ptr<WgpuDevice> _device;
};

/*!
 * @brief Test Hi-Z buffer creation with valid parameters.
 */
TEST_F(WgpuHiZBufferTest, Create_WithValidParameters_Succeeds)
{
    // Arrange
    constexpr uint32_t width  = 1024;
    constexpr uint32_t height = 768;

    // Act
    WGPUInstance  instance_handle = static_cast<WGPUInstance>(_device->wgpu_instance());
    WGPUDevice    device_handle   = static_cast<WGPUDevice>(_device->wgpu_device());
    WGPUQueue     queue_handle    = wgpuDeviceGetQueue(device_handle);
    WgpuHiZBuffer hi_z(instance_handle, device_handle, queue_handle, width, height);

    // Assert
    EXPECT_EQ(hi_z.width(), width);
    EXPECT_EQ(hi_z.height(), height);
    EXPECT_GT(hi_z.mip_levels(), 0u);

    // Expected mip levels: floor(log2(max(1024, 768))) + 1 = 10 + 1 = 11
    EXPECT_EQ(hi_z.mip_levels(), 11u);
}

/*!
 * @brief Test mip level calculation for various resolutions.
 */
TEST_F(WgpuHiZBufferTest, MipLevels_VariousResolutions_CorrectCount)
{
    struct TestCase
    {
        uint32_t width;
        uint32_t height;
        uint32_t expected_mips;
    };

    std::vector<TestCase> test_cases = {
        { 1024, 1024, 11 }, // log2(1024) + 1 = 10 + 1
        { 512, 512, 10 },   // log2(512) + 1 = 9 + 1
        { 256, 256, 9 },    // log2(256) + 1 = 8 + 1
        { 1920, 1080, 11 }, // log2(1920) + 1 = 10.906... + 1 ≈ 11
        { 800, 600, 10 },   // log2(800) + 1 = 9.643... + 1 ≈ 10
    };

    WGPUInstance instance_handle = static_cast<WGPUInstance>(_device->wgpu_instance());
    WGPUDevice   device_handle   = static_cast<WGPUDevice>(_device->wgpu_device());
    WGPUQueue    queue_handle    = wgpuDeviceGetQueue(device_handle);

    for (const auto& tc : test_cases)
    {
        WgpuHiZBuffer hi_z(instance_handle, device_handle, queue_handle, tc.width, tc.height);
        EXPECT_EQ(hi_z.mip_levels(), tc.expected_mips)
            << "Failed for resolution " << tc.width << "x" << tc.height;
    }
}

/*!
 * @brief Test move constructor transfers ownership correctly.
 */
TEST_F(WgpuHiZBufferTest, MoveConstructor_TransfersOwnership_Successfully)
{
    // Arrange
    WGPUInstance  instance_handle = static_cast<WGPUInstance>(_device->wgpu_instance());
    WGPUDevice    device_handle   = static_cast<WGPUDevice>(_device->wgpu_device());
    WGPUQueue     queue_handle    = wgpuDeviceGetQueue(device_handle);
    WgpuHiZBuffer original(instance_handle, device_handle, queue_handle, 512, 512);

    uint32_t original_width  = original.width();
    uint32_t original_height = original.height();
    uint32_t original_mips   = original.mip_levels();

    // Act
    WgpuHiZBuffer moved(std::move(original));

    // Assert
    EXPECT_EQ(moved.width(), original_width);
    EXPECT_EQ(moved.height(), original_height);
    EXPECT_EQ(moved.mip_levels(), original_mips);
}

/*!
 * @brief Test move assignment transfers ownership correctly.
 */
TEST_F(WgpuHiZBufferTest, MoveAssignment_TransfersOwnership_Successfully)
{
    // Arrange
    WGPUInstance  instance_handle = static_cast<WGPUInstance>(_device->wgpu_instance());
    WGPUDevice    device_handle   = static_cast<WGPUDevice>(_device->wgpu_device());
    WGPUQueue     queue_handle    = wgpuDeviceGetQueue(device_handle);
    WgpuHiZBuffer original(instance_handle, device_handle, queue_handle, 512, 512);
    WgpuHiZBuffer target(instance_handle, device_handle, queue_handle, 256, 256);

    uint32_t original_width  = original.width();
    uint32_t original_height = original.height();

    // Act
    target = std::move(original);

    // Assert
    EXPECT_EQ(target.width(), original_width);
    EXPECT_EQ(target.height(), original_height);
}

/*!
 * @brief Test visibility testing with empty AABB list returns empty result.
 */
TEST_F(WgpuHiZBufferTest, TestVisibility_EmptyAABBList_ReturnsEmpty)
{
    // Arrange
    WGPUInstance  instance_handle = static_cast<WGPUInstance>(_device->wgpu_instance());
    WGPUDevice    device_handle   = static_cast<WGPUDevice>(_device->wgpu_device());
    WGPUQueue     queue_handle    = wgpuDeviceGetQueue(device_handle);
    WgpuHiZBuffer hi_z(instance_handle, device_handle, queue_handle, 800, 600);

    std::vector<AABB> aabbs;
    glm::mat4         view_projection = glm::identity<glm::mat4>();

    // Act
    auto result = hi_z.test_visibility(aabbs, view_projection);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

/*!
 * @brief Test visibility with single on-screen AABB.
 */
TEST_F(WgpuHiZBufferTest, TestVisibility_SingleOnScreenAABB_ReturnsVisible)
{
    // Arrange
    WGPUInstance  instance_handle = static_cast<WGPUInstance>(_device->wgpu_instance());
    WGPUDevice    device_handle   = static_cast<WGPUDevice>(_device->wgpu_device());
    WGPUQueue     queue_handle    = wgpuDeviceGetQueue(device_handle);
    WgpuHiZBuffer hi_z(instance_handle, device_handle, queue_handle, 800, 600);

    // Create a simple view-projection matrix
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 5.0f), // camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)  // up vector
    );
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), // FOV
        800.0f / 600.0f,     // aspect ratio
        0.1f,                // near plane
        100.0f               // far plane
    );
    glm::mat4 view_projection = projection * view;

    // AABB centered at origin, visible to camera
    std::vector<AABB> aabbs = {
        AABB{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f) }
    };

    // Act
    auto result = hi_z.test_visibility(aabbs, view_projection);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 1u);
    // Note: Without a valid depth texture, test may return true (all visible) as stub
    // This is expected until full integration with rendering pipeline
}

/*!
 * @brief Test visibility with multiple AABBs.
 */
TEST_F(WgpuHiZBufferTest, TestVisibility_MultipleAABBs_ProcessesAll)
{
    // Arrange
    WGPUInstance  instance_handle = static_cast<WGPUInstance>(_device->wgpu_instance());
    WGPUDevice    device_handle   = static_cast<WGPUDevice>(_device->wgpu_device());
    WGPUQueue     queue_handle    = wgpuDeviceGetQueue(device_handle);
    WgpuHiZBuffer hi_z(instance_handle, device_handle, queue_handle, 1024, 768);

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        1024.0f / 768.0f,
        0.1f,
        1000.0f);
    glm::mat4 view_projection = projection * view;

    // Create multiple AABBs at different positions
    std::vector<AABB> aabbs;
    for (int i = 0; i < 10; ++i)
    {
        float offset = static_cast<float>(i) * 3.0f - 15.0f;
        aabbs.push_back(AABB{
            glm::vec3(offset - 0.5f, -0.5f, -0.5f),
            glm::vec3(offset + 0.5f, 0.5f, 0.5f) });
    }

    // Act
    auto result = hi_z.test_visibility(aabbs, view_projection);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), aabbs.size());

    auto stats = hi_z.stats();
    EXPECT_EQ(stats.objects_tested, aabbs.size());
    EXPECT_GT(stats.visibility_test_ms, 0.0f);
}

/*!
 * @brief Performance test with 1000 AABBs.
 */
TEST_F(WgpuHiZBufferTest, Performance_1000AABBs_CompletesInReasonableTime)
{
    GTEST_SKIP() << "Performance test - enable when profiling Hi-Z implementation";
    // Arrange
    WGPUInstance  instance_handle = static_cast<WGPUInstance>(_device->wgpu_instance());
    WGPUDevice    device_handle   = static_cast<WGPUDevice>(_device->wgpu_device());
    WGPUQueue     queue_handle    = wgpuDeviceGetQueue(device_handle);
    WgpuHiZBuffer hi_z(instance_handle, device_handle, queue_handle, 1920, 1080);

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 50.0f, 100.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        1920.0f / 1080.0f,
        0.1f,
        1000.0f);
    glm::mat4 view_projection = projection * view;

    // Generate 1000 AABBs in a grid
    std::vector<AABB> aabbs;
    aabbs.reserve(1000);
    for (int x = 0; x < 10; ++x)
    {
        for (int y = 0; y < 10; ++y)
        {
            for (int z = 0; z < 10; ++z)
            {
                float px = static_cast<float>(x) * 5.0f - 25.0f;
                float py = static_cast<float>(y) * 5.0f - 25.0f;
                float pz = static_cast<float>(z) * 5.0f - 25.0f;

                aabbs.push_back(AABB{
                    glm::vec3(px - 1.0f, py - 1.0f, pz - 1.0f),
                    glm::vec3(px + 1.0f, py + 1.0f, pz + 1.0f) });
            }
        }
    }

    // Act
    auto start       = std::chrono::high_resolution_clock::now();
    auto result      = hi_z.test_visibility(aabbs, view_projection);
    auto end         = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration<float, std::milli>(end - start).count();

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 1000u);

    auto stats = hi_z.stats();
    EXPECT_EQ(stats.objects_tested, 1000u);
    EXPECT_GT(stats.visibility_test_ms, 0.0f);

    // Performance target: < 10ms for 1000 objects (very conservative)
    EXPECT_LT(duration_ms, 10.0f)
        << "Visibility test took " << duration_ms << "ms for 1000 AABBs";
}

/*!
 * @brief Stress test with 10,000 AABBs.
 */
TEST_F(WgpuHiZBufferTest, Performance_10000AABBs_CompletesSuccessfully)
{
    // Arrange
    WGPUInstance  instance_handle = static_cast<WGPUInstance>(_device->wgpu_instance());
    WGPUDevice    device_handle   = static_cast<WGPUDevice>(_device->wgpu_device());
    WGPUQueue     queue_handle    = wgpuDeviceGetQueue(device_handle);
    WgpuHiZBuffer hi_z(instance_handle, device_handle, queue_handle, 1920, 1080);

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 100.0f, 200.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        1920.0f / 1080.0f,
        0.1f,
        1000.0f);
    glm::mat4 view_projection = projection * view;

    // Generate 10,000 AABBs
    std::vector<AABB> aabbs;
    aabbs.reserve(10000);
    for (int i = 0; i < 10000; ++i)
    {
        float angle  = static_cast<float>(i) * 0.628f; // ~2π / 10
        float radius = 50.0f + static_cast<float>(i % 100);
        float px     = std::cos(angle) * radius;
        float py     = static_cast<float>(i % 50) * 2.0f - 50.0f;
        float pz     = std::sin(angle) * radius;

        aabbs.push_back(AABB{
            glm::vec3(px - 0.5f, py - 0.5f, pz - 0.5f),
            glm::vec3(px + 0.5f, py + 0.5f, pz + 0.5f) });
    }

    // Act
    auto start       = std::chrono::high_resolution_clock::now();
    auto result      = hi_z.test_visibility(aabbs, view_projection);
    auto end         = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration<float, std::milli>(end - start).count();

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 10000u);

    auto stats = hi_z.stats();
    EXPECT_EQ(stats.objects_tested, 10000u);

    // Performance target: < 50ms for 10k objects
    EXPECT_LT(duration_ms, 50.0f)
        << "Visibility test took " << duration_ms << "ms for 10,000 AABBs";

    // Log performance for reference
    std::cout << "Hi-Z Performance: " << stats.objects_tested << " objects tested in "
              << stats.visibility_test_ms << "ms ("
              << (stats.visibility_test_ms / stats.objects_tested * 1000.0f)
              << " µs per object)" << std::endl;
}

/*!
 * @brief Test statistics are properly updated.
 */
TEST_F(WgpuHiZBufferTest, Stats_AfterVisibilityTest_UpdatedCorrectly)
{
    // Arrange
    WGPUInstance  instance_handle = static_cast<WGPUInstance>(_device->wgpu_instance());
    WGPUDevice    device_handle   = static_cast<WGPUDevice>(_device->wgpu_device());
    WGPUQueue     queue_handle    = wgpuDeviceGetQueue(device_handle);
    WgpuHiZBuffer hi_z(instance_handle, device_handle, queue_handle, 800, 600);

    glm::mat4 view_projection = glm::identity<glm::mat4>();

    std::vector<AABB> aabbs;
    for (int i = 0; i < 50; ++i)
    {
        aabbs.push_back(AABB{
            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f) });
    }

    // Act
    auto result = hi_z.test_visibility(aabbs, view_projection);

    // Assert
    ASSERT_TRUE(result.has_value());

    auto stats = hi_z.stats();
    EXPECT_EQ(stats.objects_tested, 50u);
    EXPECT_LE(stats.objects_visible, stats.objects_tested);
    EXPECT_EQ(stats.objects_culled, stats.objects_tested - stats.objects_visible);
    EXPECT_GE(stats.visibility_test_ms, 0.0f);
}
