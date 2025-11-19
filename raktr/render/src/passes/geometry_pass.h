/*!
 * @file geometry_pass.h
 * @brief Render pass for drawing visible geometry with depth output.
 */

#pragma once

#include "buffer.h"
#include "pass_context.h"
#include "render_pass.h"
#include <glm/glm.hpp>
#include <span>
#include <vector>
#include <webgpu/webgpu.h>

namespace raktr::render
{

    /*!
     * @brief Renders visible geometry to color and depth targets.
     *
     * Uses visibility results from occlusion culling pass to skip occluded objects.
     * Outputs both color (for display) and depth (for next frame's Hi-Z pyramid).
     *
     * @example
     * @code
     * auto geometry_pass = std::make_unique<GeometryPass>(
     *     vertex_buffer,
     *     index_buffer,
     *     instance_buffer,
     *     visibility_results,
     *     pipeline
     * );
     * graph.add_pass(std::move(geometry_pass));
     * @endcode
     */
    class GeometryPass final
    {
    public:
        using ContextType = PassContext; // Required for type erasure

        /*!
         * @brief Constructs geometry rendering pass.
         * @param vertex_buffer Shared vertex data for all instances.
         * @param index_buffer Shared index data for all instances.
         * @param instance_buffer Per-instance data (transforms, colors).
         * @param visibility Bitfield from occlusion pass (true = visible).
         * @param render_pipeline WebGPU render pipeline for drawing.
         * @param instance_count Total number of instances.
         * @param index_count Number of indices per instance.
         */
        GeometryPass(
            Buffer                   vertex_buffer,
            Buffer                   index_buffer,
            Buffer                   instance_buffer,
            const std::vector<bool>* visibility,
            WGPURenderPipeline       render_pipeline,
            uint32_t                 instance_count,
            uint32_t                 index_count);

        ~GeometryPass() = default;

        // Non-copyable, movable
        GeometryPass(const GeometryPass&)            = delete;
        GeometryPass& operator=(const GeometryPass&) = delete;
        GeometryPass(GeometryPass&&)                 = default;
        GeometryPass& operator=(GeometryPass&&)      = default;

        void                           execute(PassContext& ctx);
        void                           on_viewport_resize(uint32_t width, uint32_t height);
        [[nodiscard]] std::string_view name() const
        {
            return "GeometryPass";
        }

        /*!
         * @brief Update visibility results for next frame.
         * @param visibility New visibility bitfield.
         */
        void update_visibility(const std::vector<bool>* visibility);

    private:
        Buffer                   _vertex_buffer;
        Buffer                   _index_buffer;
        Buffer                   _instance_buffer;
        const std::vector<bool>* _visibility;
        WGPURenderPipeline       _render_pipeline{ nullptr };
        uint32_t                 _instance_count{ 0 };
        uint32_t                 _index_count{ 0 };
    };

} // namespace raktr::render
