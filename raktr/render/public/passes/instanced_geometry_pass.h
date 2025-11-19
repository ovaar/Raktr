/*!
 * @file instanced_geometry_pass.h
 * @brief Render pass for drawing visible geometry using GPU instancing.
 */

#pragma once

#include "buffer.h"
#include "device.h"
#include "instance_data.h"
#include "render_pass.h"
#include <glm/glm.hpp>
#include <span>
#include <vector>
#include <webgpu/webgpu.h>

namespace raktr::render
{

    /*!
     * @brief Renders visible geometry using GPU instancing with a single draw call.
     *
     * More efficient than GeometryPass for large numbers of objects. Rebuilds
     * the instance buffer each frame to contain only visible instances.
     *
     * @example
     * @code
     * auto geometry_pass = std::make_unique<InstancedGeometryPass>(
     *     device,
     *     vertex_buffer,
     *     index_buffer,
     *     instance_buffer,
     *     all_instances,
     *     visibility_results,
     *     36  // index count
     * );
     * graph.add_pass(std::move(geometry_pass));
     * @endcode
     */
    class InstancedGeometryPass final
    {
    public:
        using ContextType = PassContext; // Required for type erasure

        /*!
         * @brief Constructs instanced geometry rendering pass.
         * @param device Device for updating instance buffer.
         * @param vertex_buffer Shared vertex data for all instances.
         * @param index_buffer Shared index data for all instances.
         * @param instance_buffer GPU buffer for instance data (updated each frame).
         * @param all_instances CPU-side instance data for all objects.
         * @param visibility Bitfield from occlusion pass (true = visible).
         * @param index_count Number of indices per instance.
         */
        InstancedGeometryPass(
            Device*                          device,
            Buffer                           vertex_buffer,
            Buffer                           index_buffer,
            Buffer                           instance_buffer,
            const std::vector<InstanceData>* all_instances,
            const std::vector<bool>*         visibility,
            uint32_t                         index_count);

        ~InstancedGeometryPass() = default;

        // Non-copyable, movable
        InstancedGeometryPass(const InstancedGeometryPass&)            = delete;
        InstancedGeometryPass& operator=(const InstancedGeometryPass&) = delete;
        InstancedGeometryPass(InstancedGeometryPass&&)                 = default;
        InstancedGeometryPass& operator=(InstancedGeometryPass&&)      = default;

        // Required interface for type erasure
        void                           execute(PassContext& ctx);
        void                           on_viewport_resize(uint32_t width, uint32_t height);
        [[nodiscard]] std::string_view name() const
        {
            return "InstancedGeometryPass";
        }

        /*!
         * @brief Get number of instances drawn in last execute().
         * @return Visible instance count.
         */
        [[nodiscard]] uint32_t last_drawn_count() const
        {
            return _last_drawn_count;
        }

    private:
        Device*                          _device{ nullptr };
        Buffer                           _vertex_buffer;
        Buffer                           _index_buffer;
        Buffer                           _instance_buffer;
        const std::vector<InstanceData>* _all_instances;
        const std::vector<bool>*         _visibility;
        uint32_t                         _index_count{ 0 };
        uint32_t                         _last_drawn_count{ 0 };
    };

} // namespace raktr::render
