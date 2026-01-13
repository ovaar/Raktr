/*!
 * @file instanced_geometry_pass.h
 * @brief WebGPU instanced geometry rendering pass.
 */

#pragma once

#include "buffer.h"
#include "instance_data.h"
#include <glm/glm.hpp>
#include <string_view>
#include <vector>
#include <webgpu/webgpu.h>

namespace raktr::render::backend::wgpu
{
    // Forward declarations
    class WgpuDevice;
    struct WgpuPassContext;

    class InstancedGeometryPass final
    {
    public:
        using ContextType = WgpuPassContext; // Required for type erasure

        InstancedGeometryPass(
            WgpuDevice*                      device,
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

        void                           execute(WgpuPassContext& ctx);
        void                           on_viewport_resize(uint32_t width, uint32_t height);
        [[nodiscard]] std::string_view name() const
        {
            return "InstancedGeometryPass";
        }

        [[nodiscard]] uint32_t last_drawn_count() const
        {
            return _last_drawn_count;
        }

    private:
        WgpuDevice*                      _device{ nullptr };
        Buffer                           _vertex_buffer;
        Buffer                           _index_buffer;
        Buffer                           _instance_buffer;
        const std::vector<InstanceData>* _all_instances;
        const std::vector<bool>*         _visibility;
        uint32_t                         _index_count{ 0 };
        uint32_t                         _last_drawn_count{ 0 };
    };

} // namespace raktr::render::backend::wgpu
