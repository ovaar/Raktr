/*!
 * @file mock_device.cpp
 * @brief Implementation of MockDevice.
 */

#include "backend/mock_device.h"
#include "render_error.h"

namespace raktr::render::backend
{

std::expected<Buffer, std::error_code> 
MockDevice::create_vertex_buffer(std::span<const std::byte> data)
{
    // Validate data is not empty
    if (data.empty())
    {
        return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
    }

    // Create a mock buffer with a unique ID
    Buffer buffer(_next_buffer_id++, BufferType::Vertex);
    return buffer;
}

std::expected<Buffer, std::error_code> 
MockDevice::create_index_buffer(std::span<const std::byte> data)
{
    // Validate data is not empty
    if (data.empty())
    {
        return std::unexpected(make_error_code(RenderError::BufferCreationFailed));
    }

    // Create a mock buffer with a unique ID
    Buffer buffer(_next_buffer_id++, BufferType::Index);
    return buffer;
}

std::expected<void, std::error_code>
MockDevice::draw_indexed(const Buffer& vertex_buffer, 
                        const Buffer& index_buffer, 
                        uint32_t index_count)
{
    // Validate buffers
    if (!vertex_buffer.is_valid() || !index_buffer.is_valid())
    {
        return std::unexpected(make_error_code(RenderError::InvalidOperation));
    }

    if (index_count == 0)
    {
        return std::unexpected(make_error_code(RenderError::InvalidOperation));
    }

    // Mock implementation - no actual draw call
    return {};
}

void MockDevice::clear()
{
    // Mock implementation - no actual clear
}

void MockDevice::present()
{
    // Mock implementation - no actual present
}

} // namespace raktr::render::backend
