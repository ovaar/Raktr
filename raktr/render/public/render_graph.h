/*!
 * @file render_graph.h
 * @brief Composite pattern for managing render pass execution.
 */

#ifndef RAKTR_RENDER_RENDER_GRAPH_H
#define RAKTR_RENDER_RENDER_GRAPH_H

#include "device.h"
#include "render_pass.h"
#include <memory>
#include <vector>


namespace raktr::render
{
    /*!
     * @brief Manages execution order and lifecycle of render passes.
     *
     * Uses Composite pattern to organize passes into an execution graph.
     * Passes are executed sequentially in the order they were added.
     * Device is injected via constructor (Dependency Injection).
     *
     * **Type Erasure**: Uses type-erased RenderPass wrapper, so passes don't need
     * to inherit from a base class. Any type with the required interface works.
     *
     * @example
     * @code
     * RenderGraph graph(device);
     * // Pass concrete pass objects directly (no std::make_unique needed!)
     * graph.add_pass(HiZOcclusionPass{hi_z_buffer, aabbs, vp, results})
     *      .add_pass(GeometryPass{vb, ib, inst_buf, visibility, pipeline, count, indices})
     *      .add_pass(HiZPyramidPass{hi_z_buffer, depth_tex});
     *
     * PassContext ctx{frame_index};
     * graph.execute(ctx);
     * @endcode
     */
    class RenderGraph
    {
    public:
        /*!
         * @brief Construct a RenderGraph with device dependency injection.
         * @param device GPU device view.
         */
        explicit RenderGraph(DeviceView device);

        /*!
         * @brief Add a pass to the execution graph (type-erased).
         * @tparam PassType Concrete pass type (e.g., GeometryPass, HiZOcclusionPass).
         * @param pass Pass to add (moved into type-erased storage).
         * @return Reference to this graph for chaining.
         *
         * **Requirements** (duck typing):
         * - PassType::ContextType typedef must exist
         * - PassType must have: std::string_view name() const
         * - PassType must have: void execute(ContextType& ctx)
         * - PassType must have: void on_viewport_resize(uint32_t, uint32_t)
         *
         * @example
         * graph.add_pass(GeometryPass{vb, ib, inst_buf, vis, pipeline, 100, 36});
         */
        template <typename PassType>
        RenderGraph& add_pass(PassType pass)
        {
            _passes.push_back(RenderPass(std::move(pass)));
            return *this;
        }

        /*!
         * @brief Execute all passes in order.
         * @tparam PassContextType Backend-specific context type.
         * @param ctx Context containing frame state and resources.
         */
        template <typename PassContextType>
        void execute(PassContextType& ctx)
        {
            for (auto& pass : _passes)
            {
                pass.execute(ctx);
            }
        }

        /*!
         * @brief Notify all passes of viewport resize.
         * @param width New viewport width.
         * @param height New viewport height.
         */
        void on_viewport_resize(uint32_t width, uint32_t height);

        /*!
         * @brief Get number of passes in the graph.
         * @return Pass count.
         */
        [[nodiscard]] size_t pass_count() const
        {
            return _passes.size();
        }

        /*!
         * @brief Clear all passes from the graph.
         */
        void clear();

        /*!
         * @brief Get the injected device.
         * @return Device view.
         */
        [[nodiscard]] DeviceView device() const
        {
            return _device;
        }

    private:
        DeviceView              _device; // Non-owning, injected dependency
        std::vector<RenderPass> _passes; // Type-erased passes (value semantics!)
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_RENDER_GRAPH_H
