/*!
 * @file render_graph.cpp
 * @brief Implementation of RenderGraph composite.
 */

#include "render_graph.h"
#include "device.h"

namespace raktr::render
{

    RenderGraph::RenderGraph(Device* device)
        : _device(device)
    {
    }

    // Note: add_pass() is now a template method defined in the header

    // Note: execute() is now a template method defined in the header

    void RenderGraph::on_viewport_resize(uint32_t width, uint32_t height)
    {
        for (auto& pass : _passes)
        {
            pass.on_viewport_resize(width, height);
        }
    }

    void RenderGraph::clear()
    {
        _passes.clear();
    }

} // namespace raktr::render
