/*!
 * @file instanced_geometry_pass.cpp
 * @brief WebGPU instanced geometry rendering pass implementation.
 */

#include "instanced_geometry_pass.h"
#include "device.h"
#include <spdlog/spdlog.h>

namespace raktr::render::backend::wgpu
{

    InstancedGeometryPass::InstancedGeometryPass(Device*                          device,
                                                 Buffer                           vertex_buffer,
                                                 Buffer                           index_buffer,
                                                 Buffer                           instance_buffer,
                                                 const std::vector<InstanceData>* all_instances,
                                                 const std::vector<bool>*         visibility,
                                                 uint32_t                         index_count)
        : _device(device),
          _vertex_buffer(vertex_buffer),
          _index_buffer(index_buffer),
          _instance_buffer(instance_buffer),
          _all_instances(all_instances),
          _visibility(visibility),
          _index_count(index_count),
          _last_drawn_count(0)
    {
    }

    void InstancedGeometryPass::execute([[maybe_unused]] WgpuPassContext& ctx)
    {
        // Build instance data for visible objects only
        std::vector<InstanceData> visible_instances;
        visible_instances.reserve(_all_instances->size());

        for (size_t i = 0; i < _all_instances->size(); ++i)
        {
            // Check visibility (with bounds check)
            bool is_visible = (i < _visibility->size()) ? (*_visibility)[i] : true;

            if (is_visible)
            {
                visible_instances.push_back((*_all_instances)[i]);
            }
        }

        _last_drawn_count = static_cast<uint32_t>(visible_instances.size());

        // Update instance buffer with visible instances
        if (!visible_instances.empty())
        {
            auto update_result = _device->update_instance_buffer(
                _instance_buffer,
                std::as_bytes(std::span(visible_instances)));

            if (!update_result.has_value())
            {
                spdlog::error("InstancedGeometryPass: Failed to update instance buffer");
                return;
            }

            // Draw all visible instances in one call
            auto draw_result = _device->draw_indexed_instanced(
                _vertex_buffer,
                _index_buffer,
                _instance_buffer,
                _index_count,
                _last_drawn_count);

            if (!draw_result.has_value())
            {
                spdlog::error("InstancedGeometryPass: Failed to draw instances");
            }
        }

        // Calculate and log culling statistics
        uint32_t total_count  = static_cast<uint32_t>(_all_instances->size());
        uint32_t culled_count = total_count - _last_drawn_count;
        float    culling_rate = total_count > 0
                                    ? (100.0f * culled_count / total_count)
                                    : 0.0f;

        spdlog::debug("InstancedGeometryPass: Drew {}/{} instances ({:.1f}% culled)",
                      _last_drawn_count,
                      total_count,
                      culling_rate);
    }

    void InstancedGeometryPass::on_viewport_resize([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
    {
        // Nothing to do - render targets are managed by caller
    }

} // namespace raktr::render::backend::wgpu
