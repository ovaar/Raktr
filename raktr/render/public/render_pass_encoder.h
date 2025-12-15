/*!
 * @file render_pass_encoder.h
 * @brief Type-erased render pass encoder for recording graphics commands.
 */

#ifndef RAKTR_RENDER_RENDER_PASS_ENCODER_H
#define RAKTR_RENDER_RENDER_PASS_ENCODER_H

#include "buffer.h"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>

namespace raktr::render
{
    // Forward declaration
    class RenderPipeline;

    /*!
     * @brief Load operation for render pass attachments.
     */
    enum class LoadOp
    {
        Load,  ///< Load existing attachment contents
        Clear, ///< Clear attachment to specified value
    };

    /*!
     * @brief Store operation for render pass attachments.
     */
    enum class StoreOp
    {
        Store,   ///< Store render pass results to attachment
        Discard, ///< Discard render pass results (for transient attachments)
    };

    /*!
     * @brief Clear color value.
     */
    struct ClearColor
    {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;
        double a = 1.0;
    };

    /*!
     * @brief Color attachment descriptor for render pass.
     */
    struct RenderPassColorAttachment
    {
        void*      view     = nullptr; ///< Texture view (backend-specific, e.g., WGPUTextureView)
        void*      resolve  = nullptr; ///< Resolve target for MSAA (optional)
        LoadOp     load_op  = LoadOp::Clear;
        StoreOp    store_op = StoreOp::Store;
        ClearColor clear_color;
    };

    /*!
     * @brief Depth/stencil attachment descriptor for render pass.
     */
    struct RenderPassDepthStencilAttachment
    {
        void*    view                = nullptr; ///< Depth/stencil texture view
        LoadOp   depth_load_op       = LoadOp::Clear;
        StoreOp  depth_store_op      = StoreOp::Store;
        float    depth_clear_value   = 1.0f;
        bool     depth_read_only     = false;
        LoadOp   stencil_load_op     = LoadOp::Clear;
        StoreOp  stencil_store_op    = StoreOp::Store;
        uint32_t stencil_clear_value = 0;
        bool     stencil_read_only   = false;
    };

    /*!
     * @brief Render pass descriptor.
     */
    struct RenderPassDescriptor
    {
        std::array<RenderPassColorAttachment, 8>        color_attachments;
        uint32_t                                        color_attachment_count = 0;
        std::optional<RenderPassDepthStencilAttachment> depth_stencil_attachment;
    };

    /*!
     * @brief Type-erased render pass encoder for recording graphics commands.
     *
     * RenderPassEncoder is created from CommandEncoder.begin_render_pass() and provides
     * methods for recording draw calls, setting pipeline state, and binding resources.
     *
     * The pass must be ended with end() before the command buffer can be finished.
     *
     * @example
     * RenderPassDescriptor desc;
     * desc.color_attachments[0].view = surface_view;
     * desc.color_attachments[0].load_op = LoadOp::Clear;
     * desc.color_attachments[0].clear_color = {0.0, 0.0, 0.0, 1.0};
     * desc.color_attachment_count = 1;
     *
     * auto pass = encoder.begin_render_pass(desc);
     * pass.set_vertex_buffer(0, vertex_buffer);
     * pass.draw(3, 1, 0, 0);
     * pass.end();
     */
    class RenderPassEncoder
    {
    public:
        /*!
         * @brief Construct a RenderPassEncoder from any concrete encoder type.
         * @param encoder_impl Concrete encoder instance (WgpuRenderPassEncoder, etc.).
         */
        template <typename T>
        RenderPassEncoder(T encoder_impl)
            : _impl(std::make_unique<Model<T>>(std::move(encoder_impl)))
        {
        }

        // Non-copyable
        RenderPassEncoder(const RenderPassEncoder&)            = delete;
        RenderPassEncoder& operator=(const RenderPassEncoder&) = delete;

        // Movable
        RenderPassEncoder(RenderPassEncoder&&) noexcept            = default;
        RenderPassEncoder& operator=(RenderPassEncoder&&) noexcept = default;

        ~RenderPassEncoder() = default;

        /*!
         * @brief Set the render pipeline for subsequent draw calls.
         *
         * Configures the graphics pipeline state (shaders, vertex layout, blend mode, etc.)
         * for all draw calls that follow until another pipeline is set.
         *
         * @param pipeline Render pipeline to bind.
         *
         * @example
         * RenderPassEncoder pass = encoder.begin_render_pass(desc);
         * pass.set_pipeline(my_pipeline);
         * pass.set_vertex_buffer(0, vertex_buffer);
         * pass.draw(vertex_count, 1, 0, 0);
         * pass.end();
         */
        void set_pipeline(const RenderPipeline& pipeline) const
        {
            _impl->do_set_pipeline(pipeline);
        }

        /*!
         * @brief Set vertex buffer for rendering.
         * @param slot Vertex buffer binding slot.
         * @param buffer Vertex buffer to bind.
         * @param offset Byte offset into buffer.
         * @param size Size of buffer region to bind (0 = entire buffer).
         */
        void set_vertex_buffer(uint32_t slot, Buffer buffer, uint64_t offset = 0, uint64_t size = 0) const
        {
            _impl->do_set_vertex_buffer(slot, buffer, offset, size);
        }

        /*!
         * @brief Set index buffer for indexed rendering.
         * @param buffer Index buffer to bind.
         * @param offset Byte offset into buffer.
         * @param size Size of buffer region to bind (0 = entire buffer).
         */
        void set_index_buffer(Buffer buffer, uint64_t offset = 0, uint64_t size = 0) const
        {
            _impl->do_set_index_buffer(buffer, offset, size);
        }

        /*!
         * @brief Draw non-indexed geometry.
         * @param vertex_count Number of vertices to draw.
         * @param instance_count Number of instances to draw.
         * @param first_vertex Offset to first vertex.
         * @param first_instance Offset to first instance.
         */
        void draw(uint32_t vertex_count, uint32_t instance_count = 1, uint32_t first_vertex = 0, uint32_t first_instance = 0) const
        {
            _impl->do_draw(vertex_count, instance_count, first_vertex, first_instance);
        }

        /*!
         * @brief Draw indexed geometry.
         * @param index_count Number of indices to draw.
         * @param instance_count Number of instances to draw.
         * @param first_index Offset to first index.
         * @param base_vertex Vertex offset added to each index.
         * @param first_instance Offset to first instance.
         */
        void draw_indexed(uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0, int32_t base_vertex = 0, uint32_t first_instance = 0) const
        {
            _impl->do_draw_indexed(index_count, instance_count, first_index, base_vertex, first_instance);
        }

        /*!
         * @brief End the render pass.
         *
         * Must be called before finishing the command encoder.
         * After calling end(), this RenderPassEncoder object becomes invalid.
         */
        void end() const
        {
            _impl->do_end();
        }

    private:
        struct Concept
        {
            virtual ~Concept()                                                                                                                                    = default;
            virtual void do_set_pipeline(const RenderPipeline& pipeline) const                                                                                    = 0;
            virtual void do_set_vertex_buffer(uint32_t slot, Buffer buffer, uint64_t offset, uint64_t size) const                                                 = 0;
            virtual void do_set_index_buffer(Buffer buffer, uint64_t offset, uint64_t size) const                                                                 = 0;
            virtual void do_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) const                            = 0;
            virtual void do_draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t base_vertex, uint32_t first_instance) const = 0;
            virtual void do_end() const                                                                                                                           = 0;
        };

        template <typename T>
        struct Model : Concept
        {
            explicit Model(T encoder_impl)
                : _encoder(std::move(encoder_impl))
            {
            }

            void do_set_pipeline(const RenderPipeline& pipeline) const override
            {
                _encoder.set_pipeline(pipeline);
            }

            void do_set_vertex_buffer(uint32_t slot, Buffer buffer, uint64_t offset, uint64_t size) const override
            {
                _encoder.set_vertex_buffer(slot, buffer, offset, size);
            }

            void do_set_index_buffer(Buffer buffer, uint64_t offset, uint64_t size) const override
            {
                _encoder.set_index_buffer(buffer, offset, size);
            }

            void do_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) const override
            {
                _encoder.draw(vertex_count, instance_count, first_vertex, first_instance);
            }

            void do_draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t base_vertex, uint32_t first_instance) const override
            {
                _encoder.draw_indexed(index_count, instance_count, first_index, base_vertex, first_instance);
            }

            void do_end() const override
            {
                _encoder.end();
            }

            mutable T _encoder;
        };

        std::unique_ptr<Concept> _impl;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_RENDER_PASS_ENCODER_H
