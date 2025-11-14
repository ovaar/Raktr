/*!
 * @file instance_data.h
 * @brief Per-instance data structure for instanced rendering.
 */

#ifndef RAKTR_RENDER_INSTANCE_DATA_H
#define RAKTR_RENDER_INSTANCE_DATA_H

#include <glm/glm.hpp>
#include <span>

namespace raktr::render
{
    /*!
     * @brief Per-instance data for instanced rendering.
     *
     * Contains transformation and color information that varies per instance.
     * Layout matches WGSL shader expectations:
     * - locations 1-4: model matrix (4× vec4)
     * - location 5: color (vec4)
     *
     * Total size: 80 bytes (64 + 16, naturally aligned)
     *
     * @example
     * std::vector<InstanceData> instances(100);
     * for (int i = 0; i < 100; ++i) {
     *     instances[i].model_matrix = glm::translate(glm::mat4(1.0f), position);
     *     instances[i].color = glm::vec4(r, g, b, 1.0f);
     * }
     * auto instance_buffer = device->create_instance_buffer(as_bytes(span(instances)));
     */
    struct InstanceData
    {
        glm::mat4 model_matrix; // 64 bytes (4×4 floats)
        glm::vec4 color;        // 16 bytes (4 floats)
        // Total: 80 bytes

        /*!
         * @brief Convert to byte span for GPU upload.
         * @return Read-only view of this instance as bytes.
         */
        [[nodiscard]] std::span<const std::byte> to_bytes() const
        {
            return std::as_bytes(std::span(this, 1));
        }
    };

    // Helper to convert vector of instances to bytes
    inline std::span<const std::byte> as_instance_bytes(std::span<const InstanceData> instances)
    {
        return std::as_bytes(instances);
    }

} // namespace raktr::render

#endif // RAKTR_RENDER_INSTANCE_DATA_H
