/*!
 * @file test_instance_rendering.cpp
 * @brief Unit tests for instance rendering capability.
 */

#include "device.h"
#include "device_capabilities.h"
#include "instance_data.h"
#include "render_context.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

using namespace raktr::render;

/*!
 * @brief Test fixture for instance rendering tests.
 */
class InstanceRenderingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create fake render context
        _ctx = create_render_context();
        RenderConfig config{ .backend = BackendType::Soft };
        auto         result = _ctx->initialize(config);
        ASSERT_TRUE(result.has_value()) << "Failed to initialize render context";
        _device = _ctx->device();
    }

    std::unique_ptr<RenderContext> _ctx;
    Device*                        _device = nullptr;
};

// ============================================================================
// Triple-A: create_instance_buffer Tests
// ============================================================================

TEST_F(InstanceRenderingTest, CreateInstanceBuffer_WithValidData_ReturnsValidBuffer)
{
    // Arrange: Prepare instance data
    std::vector<InstanceData> instances;
    instances.push_back({
        glm::mat4(1.0f),                  // Identity matrix
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) // Red color
    });

    auto data_span = std::as_bytes(std::span(instances));

    // Act: Create instance buffer
    auto result = _device->create_instance_buffer(data_span);

    // Assert: Buffer creation succeeded
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().is_valid());
    EXPECT_EQ(result.value().type(), BufferType::Instance);
}

TEST_F(InstanceRenderingTest, CreateInstanceBuffer_WithMultipleInstances_ReturnsValidBuffer)
{
    // Arrange: Prepare 100 instances
    std::vector<InstanceData> instances;
    for (int i = 0; i < 100; ++i)
    {
        float offset = static_cast<float>(i) * 2.0f;
        auto  model  = glm::translate(glm::mat4(1.0f), glm::vec3(offset, 0.0f, 0.0f));
        float t      = static_cast<float>(i) / 99.0f;
        auto  color  = glm::vec4(t, 1.0f - t, 0.5f, 1.0f);
        instances.push_back({ model, color });
    }

    auto data_span = std::as_bytes(std::span(instances));

    // Act: Create instance buffer
    auto result = _device->create_instance_buffer(data_span);

    // Assert: Buffer creation succeeded
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().is_valid());
    EXPECT_EQ(result.value().type(), BufferType::Instance);
}

TEST_F(InstanceRenderingTest, CreateInstanceBuffer_WithEmptyData_ReturnsError)
{
    // Arrange: Empty data span
    std::vector<std::byte> empty_data;
    auto                   data_span = std::span(empty_data);

    // Act: Attempt to create instance buffer
    auto result = _device->create_instance_buffer(data_span);

    // Assert: Buffer creation failed
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Triple-A: update_instance_buffer Tests
// ============================================================================

TEST_F(InstanceRenderingTest, UpdateInstanceBuffer_WithValidData_Succeeds)
{
    // Arrange: Create initial instance buffer
    std::vector<InstanceData> instances;
    instances.push_back({ glm::mat4(1.0f),
                          glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) });

    auto buffer_result = _device->create_instance_buffer(std::as_bytes(std::span(instances)));
    ASSERT_TRUE(buffer_result.has_value());
    auto buffer = buffer_result.value();

    // Modify instance data
    instances[0].model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f));
    instances[0].color        = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    // Act: Update instance buffer
    auto update_result = _device->update_instance_buffer(buffer, std::as_bytes(std::span(instances)));

    // Assert: Update succeeded
    EXPECT_TRUE(update_result.has_value());
}

TEST_F(InstanceRenderingTest, UpdateInstanceBuffer_WithInvalidBuffer_ReturnsError)
{
    // Arrange: Invalid buffer + valid data
    Buffer                    invalid_buffer;
    std::vector<InstanceData> instances;
    instances.push_back({ glm::mat4(1.0f), glm::vec4(1.0f) });

    // Act: Attempt to update invalid buffer
    auto result = _device->update_instance_buffer(invalid_buffer, std::as_bytes(std::span(instances)));

    // Assert: Update failed
    EXPECT_FALSE(result.has_value());
}

TEST_F(InstanceRenderingTest, UpdateInstanceBuffer_WithWrongBufferType_ReturnsError)
{
    // Arrange: Create vertex buffer (wrong type)
    float vertices[]           = { 0.0f, 0.0f, 0.0f };
    auto  vertex_buffer_result = _device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vertex_buffer_result.has_value());
    auto vertex_buffer = vertex_buffer_result.value();

    std::vector<InstanceData> instances;
    instances.push_back({ glm::mat4(1.0f), glm::vec4(1.0f) });

    // Act: Attempt to update vertex buffer as instance buffer
    auto result = _device->update_instance_buffer(vertex_buffer, std::as_bytes(std::span(instances)));

    // Assert: Update failed (wrong buffer type)
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Triple-A: draw_indexed_instanced Tests
// ============================================================================

TEST_F(InstanceRenderingTest, DrawIndexedInstanced_WithValidBuffers_Succeeds)
{
    // Arrange: Create geometry buffers (triangle)
    float vertices[] = {
        0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f
    };
    uint32_t indices[] = { 0, 1, 2 };

    auto vb_result = _device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vb_result.has_value());
    auto vertex_buffer = vb_result.value();

    auto ib_result = _device->create_index_buffer(std::as_bytes(std::span(indices)));
    ASSERT_TRUE(ib_result.has_value());
    auto index_buffer = ib_result.value();

    // Create instance buffer (10 instances)
    std::vector<InstanceData> instances;
    for (int i = 0; i < 10; ++i)
    {
        float offset = static_cast<float>(i) * 0.2f;
        auto  model  = glm::translate(glm::mat4(1.0f), glm::vec3(offset, 0.0f, 0.0f));
        instances.push_back({ model, glm::vec4(1.0f) });
    }

    auto inst_result = _device->create_instance_buffer(std::as_bytes(std::span(instances)));
    ASSERT_TRUE(inst_result.has_value());
    auto instance_buffer = inst_result.value();

    // Act: Draw instanced
    auto draw_result = _device->draw_indexed_instanced(
        vertex_buffer,
        index_buffer,
        instance_buffer,
        3, // 3 indices (triangle)
        10 // 10 instances
    );

    // Assert: Draw succeeded
    EXPECT_TRUE(draw_result.has_value());
}

TEST_F(InstanceRenderingTest, DrawIndexedInstanced_WithInvalidVertexBuffer_ReturnsError)
{
    // Arrange: Invalid vertex buffer, valid index and instance buffers
    Buffer invalid_vb;

    uint32_t indices[] = { 0, 1, 2 };
    auto     ib_result = _device->create_index_buffer(std::as_bytes(std::span(indices)));
    ASSERT_TRUE(ib_result.has_value());
    auto index_buffer = ib_result.value();

    std::vector<InstanceData> instances;
    instances.push_back({ glm::mat4(1.0f), glm::vec4(1.0f) });
    auto inst_result = _device->create_instance_buffer(std::as_bytes(std::span(instances)));
    ASSERT_TRUE(inst_result.has_value());
    auto instance_buffer = inst_result.value();

    // Act: Attempt to draw with invalid vertex buffer
    auto result = _device->draw_indexed_instanced(invalid_vb, index_buffer, instance_buffer, 3, 1);

    // Assert: Draw failed
    EXPECT_FALSE(result.has_value());
}

TEST_F(InstanceRenderingTest, DrawIndexedInstanced_WithInvalidIndexBuffer_ReturnsError)
{
    // Arrange: Valid vertex buffer, invalid index buffer, valid instance buffer
    float vertices[] = { 0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f };
    auto  vb_result  = _device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vb_result.has_value());
    auto vertex_buffer = vb_result.value();

    Buffer invalid_ib;

    std::vector<InstanceData> instances;
    instances.push_back({ glm::mat4(1.0f), glm::vec4(1.0f) });
    auto inst_result = _device->create_instance_buffer(std::as_bytes(std::span(instances)));
    ASSERT_TRUE(inst_result.has_value());
    auto instance_buffer = inst_result.value();

    // Act: Attempt to draw with invalid index buffer
    auto result = _device->draw_indexed_instanced(vertex_buffer, invalid_ib, instance_buffer, 3, 1);

    // Assert: Draw failed
    EXPECT_FALSE(result.has_value());
}

TEST_F(InstanceRenderingTest, DrawIndexedInstanced_WithInvalidInstanceBuffer_ReturnsError)
{
    // Arrange: Valid geometry buffers, invalid instance buffer
    float vertices[] = { 0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f };
    auto  vb_result  = _device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vb_result.has_value());
    auto vertex_buffer = vb_result.value();

    uint32_t indices[] = { 0, 1, 2 };
    auto     ib_result = _device->create_index_buffer(std::as_bytes(std::span(indices)));
    ASSERT_TRUE(ib_result.has_value());
    auto index_buffer = ib_result.value();

    Buffer invalid_inst;

    // Act: Attempt to draw with invalid instance buffer
    auto result = _device->draw_indexed_instanced(vertex_buffer, index_buffer, invalid_inst, 3, 1);

    // Assert: Draw failed
    EXPECT_FALSE(result.has_value());
}

TEST_F(InstanceRenderingTest, DrawIndexedInstanced_WithZeroIndexCount_ReturnsError)
{
    // Arrange: Valid buffers but zero index count
    float vertices[] = { 0.0f, 0.5f, 0.0f };
    auto  vb_result  = _device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vb_result.has_value());
    auto vertex_buffer = vb_result.value();

    uint32_t indices[] = { 0 };
    auto     ib_result = _device->create_index_buffer(std::as_bytes(std::span(indices)));
    ASSERT_TRUE(ib_result.has_value());
    auto index_buffer = ib_result.value();

    std::vector<InstanceData> instances;
    instances.push_back({ glm::mat4(1.0f), glm::vec4(1.0f) });
    auto inst_result = _device->create_instance_buffer(std::as_bytes(std::span(instances)));
    ASSERT_TRUE(inst_result.has_value());
    auto instance_buffer = inst_result.value();

    // Act: Attempt to draw with zero index count
    auto result = _device->draw_indexed_instanced(vertex_buffer, index_buffer, instance_buffer, 0, 1);

    // Assert: Draw failed
    EXPECT_FALSE(result.has_value());
}

TEST_F(InstanceRenderingTest, DrawIndexedInstanced_WithZeroInstanceCount_ReturnsError)
{
    // Arrange: Valid buffers but zero instance count
    float vertices[] = { 0.0f, 0.5f, 0.0f };
    auto  vb_result  = _device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vb_result.has_value());
    auto vertex_buffer = vb_result.value();

    uint32_t indices[] = { 0, 1, 2 };
    auto     ib_result = _device->create_index_buffer(std::as_bytes(std::span(indices)));
    ASSERT_TRUE(ib_result.has_value());
    auto index_buffer = ib_result.value();

    std::vector<InstanceData> instances;
    instances.push_back({ glm::mat4(1.0f), glm::vec4(1.0f) });
    auto inst_result = _device->create_instance_buffer(std::as_bytes(std::span(instances)));
    ASSERT_TRUE(inst_result.has_value());
    auto instance_buffer = inst_result.value();

    // Act: Attempt to draw with zero instance count
    auto result = _device->draw_indexed_instanced(vertex_buffer, index_buffer, instance_buffer, 3, 0);

    // Assert: Draw failed
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Triple-A: InstancingOps Capability Tests
// ============================================================================

TEST_F(InstanceRenderingTest, InstancingOpsCapability_IsAvailable)
{
    // Act & Assert: Get capability (throws if not supported)
    EXPECT_NO_THROW({
        auto capability = _device->capability<capabilities::InstancingOps>();
    });
}

TEST_F(InstanceRenderingTest, InstancingOpsCapability_CreateInstanceBufferWorks)
{
    // Arrange: Get capability
    auto capability = _device->capability<capabilities::InstancingOps>();

    std::vector<InstanceData> instances;
    instances.push_back({ glm::mat4(1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) });

    // Act: Create buffer via capability
    auto result = capability.create_instance_buffer(std::as_bytes(std::span(instances)));

    // Assert: Buffer created successfully
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().is_valid());
    EXPECT_EQ(result.value().type(), BufferType::Instance);
}

TEST_F(InstanceRenderingTest, InstancingOpsCapability_UpdateInstanceBufferWorks)
{
    // Arrange: Get capability and create buffer
    auto capability = _device->capability<capabilities::InstancingOps>();

    std::vector<InstanceData> instances;
    instances.push_back({ glm::mat4(1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) });
    auto buffer_result = capability.create_instance_buffer(std::as_bytes(std::span(instances)));
    ASSERT_TRUE(buffer_result.has_value());
    auto buffer = buffer_result.value();

    // Modify data
    instances[0].color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    // Act: Update buffer via capability
    auto result = capability.update_instance_buffer(buffer, std::as_bytes(std::span(instances)));

    // Assert: Update succeeded
    EXPECT_TRUE(result.has_value());
}

TEST_F(InstanceRenderingTest, InstancingOpsCapability_DrawIndexedInstancedWorks)
{
    // Arrange: Get capability and create all buffers
    auto capability = _device->capability<capabilities::InstancingOps>();

    float vertices[] = { 0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f };
    auto  vb_result  = _device->create_vertex_buffer(std::as_bytes(std::span(vertices)));
    ASSERT_TRUE(vb_result.has_value());
    auto vertex_buffer = vb_result.value();

    uint32_t indices[] = { 0, 1, 2 };
    auto     ib_result = _device->create_index_buffer(std::as_bytes(std::span(indices)));
    ASSERT_TRUE(ib_result.has_value());
    auto index_buffer = ib_result.value();

    std::vector<InstanceData> instances;
    instances.push_back({ glm::mat4(1.0f), glm::vec4(1.0f) });
    auto inst_result = capability.create_instance_buffer(std::as_bytes(std::span(instances)));
    ASSERT_TRUE(inst_result.has_value());
    auto instance_buffer = inst_result.value();

    // Act: Draw via capability
    auto result = capability.draw_indexed_instanced(
        vertex_buffer, index_buffer, instance_buffer, 3, 1);

    // Assert: Draw succeeded
    EXPECT_TRUE(result.has_value());
}

// ============================================================================
// Triple-A: InstanceData Helper Tests
// ============================================================================

TEST_F(InstanceRenderingTest, InstanceData_ToBytes_ReturnsCorrectSize)
{
    // Arrange: Create instance data
    InstanceData instance{
        glm::mat4(1.0f),
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)
    };

    // Act: Convert to bytes
    auto bytes = instance.to_bytes();

    // Assert: Size is 80 bytes (64 for mat4 + 16 for vec4)
    EXPECT_EQ(bytes.size(), 80);
}

TEST_F(InstanceRenderingTest, InstanceData_ToBytes_PreservesMatrixData)
{
    // Arrange: Create instance with specific matrix
    auto         model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    InstanceData instance{ model, glm::vec4(1.0f) };

    // Act: Convert to bytes
    auto bytes = instance.to_bytes();

    // Assert: Can read back matrix data
    const float* matrix_data = reinterpret_cast<const float*>(bytes.data());
    EXPECT_FLOAT_EQ(matrix_data[12], 1.0f); // Translation X
    EXPECT_FLOAT_EQ(matrix_data[13], 2.0f); // Translation Y
    EXPECT_FLOAT_EQ(matrix_data[14], 3.0f); // Translation Z
}

TEST_F(InstanceRenderingTest, InstanceData_ToBytes_PreservesColorData)
{
    // Arrange: Create instance with specific color
    InstanceData instance{
        glm::mat4(1.0f),
        glm::vec4(0.25f, 0.5f, 0.75f, 1.0f)
    };

    // Act: Convert to bytes
    auto bytes = instance.to_bytes();

    // Assert: Can read back color data (after matrix)
    const float* color_data = reinterpret_cast<const float*>(bytes.data() + 64);
    EXPECT_FLOAT_EQ(color_data[0], 0.25f); // R
    EXPECT_FLOAT_EQ(color_data[1], 0.5f);  // G
    EXPECT_FLOAT_EQ(color_data[2], 0.75f); // B
    EXPECT_FLOAT_EQ(color_data[3], 1.0f);  // A
}
