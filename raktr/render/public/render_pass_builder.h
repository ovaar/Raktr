/*!
 * @file render_pass_builder.h
 * @brief Fluent API for constructing render pass descriptors.
 */

#ifndef RAKTR_RENDER_RENDER_PASS_BUILDER_H
#define RAKTR_RENDER_RENDER_PASS_BUILDER_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>


// Use webgpu.h directly since we need LoadOp/StoreOp enums
#include <webgpu/webgpu.h>

namespace raktr::render
{
    /*!
     * @brief Fluent builder for WebGPU render pass descriptors.
     *
     * Provides a type-safe, self-documenting API for constructing render passes
     * with explicit load/store operations and clear values.
     *
     * @example
     * auto render_pass = RenderPassBuilder()
     *     .color_attachment(hdr_target, WGPULoadOp_Load)
     *     .depth_attachment(depth_target, WGPULoadOp_Load)
     *     .label("GeometryPass")
     *     .begin(cmd_encoder);
     */
    class RenderPassBuilder
    {
    public:
        /*!
         * @brief Configure color attachment.
         * @param target Render target texture view.
         * @param load_op Load operation (Clear or Load).
         * @param clear_color Clear color (used if load_op is Clear).
         * @return Reference to this builder for chaining.
         */
        RenderPassBuilder& color_attachment(
            WGPUTextureView      target,
            WGPULoadOp           load_op,
            std::array<float, 4> clear_color = { 0.0f, 0.0f, 0.0f, 1.0f });

        /*!
         * @brief Configure depth attachment.
         * @param target Depth texture view.
         * @param load_op Load operation (Clear or Load).
         * @param clear_depth Clear depth value (used if load_op is Clear).
         * @return Reference to this builder for chaining.
         */
        RenderPassBuilder& depth_attachment(
            WGPUTextureView target,
            WGPULoadOp      load_op,
            float           clear_depth = 1.0f);

        /*!
         * @brief Set render pass label for debugging.
         * @param name Human-readable pass name.
         * @return Reference to this builder for chaining.
         */
        RenderPassBuilder& label(std::string_view name);

        /*!
         * @brief Begin the render pass and return encoder.
         * @param encoder Command encoder to record into.
         * @return Render pass encoder. Caller must call wgpuRenderPassEncoderEnd().
         */
        [[nodiscard]] WGPURenderPassEncoder begin(WGPUCommandEncoder encoder);

    private:
        struct ColorAttachmentDesc
        {
            WGPUTextureView      view        = nullptr;
            WGPULoadOp           load_op     = WGPULoadOp_Load;
            std::array<float, 4> clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
        };

        struct DepthAttachmentDesc
        {
            WGPUTextureView view        = nullptr;
            WGPULoadOp      load_op     = WGPULoadOp_Load;
            float           clear_depth = 1.0f;
        };

        std::optional<ColorAttachmentDesc> _color;
        std::optional<DepthAttachmentDesc> _depth;
        std::string                        _label;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_RENDER_PASS_BUILDER_H
