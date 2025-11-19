/*!
 * @file hi_z_pyramid_pass.h
 * @brief Render pass for building Hi-Z depth pyramid for next frame.
 */

#pragma once

#include "occlusion/hi_z_buffer.h"
#include "render_pass.h"
#include <webgpu/webgpu.h>

namespace raktr::render
{

    /*!
     * @brief Builds Hi-Z depth pyramid from current frame's depth buffer.
     *
     * Uses compute shader to generate mipmap chain where each level stores
     * maximum depth of 2x2 block from previous level. This pyramid will be
     * used by next frame's occlusion culling pass (temporal occlusion).
     *
     * @note This pass runs AFTER geometry has been rendered to depth buffer.
     *
     * @example
     * @code
     * auto pyramid_pass = std::make_unique<HiZPyramidPass>(
     *     hi_z_buffer.get(),
     *     depth_texture
     * );
     * graph.add_pass(std::move(pyramid_pass));
     * @endcode
     */
    class HiZPyramidPass final
    {
    public:
        using ContextType = PassContext; // Required for type erasure

        /*!
         * @brief Constructs Hi-Z pyramid generation pass.
         * @param hi_z_buffer Hi-Z buffer implementation (not owned).
         * @param depth_texture Depth texture from geometry pass.
         */
        HiZPyramidPass(
            occlusion::HiZBuffer* hi_z_buffer,
            WGPUTexture           depth_texture);

        ~HiZPyramidPass() = default;

        // Non-copyable, movable
        HiZPyramidPass(const HiZPyramidPass&)            = delete;
        HiZPyramidPass& operator=(const HiZPyramidPass&) = delete;
        HiZPyramidPass(HiZPyramidPass&&)                 = default;
        HiZPyramidPass& operator=(HiZPyramidPass&&)      = default;

        // Required interface for type erasure
        void                           execute(PassContext& ctx);
        void                           on_viewport_resize(uint32_t width, uint32_t height);
        [[nodiscard]] std::string_view name() const
        {
            return "HiZPyramidPass";
        }

        /*!
         * @brief Update depth texture for next frame.
         * @param depth_texture New depth texture.
         */
        void update_depth_texture(WGPUTexture depth_texture);

    private:
        occlusion::HiZBuffer* _hi_z_buffer{ nullptr };
        WGPUTexture           _depth_texture{ nullptr };
    };

} // namespace raktr::render
