/*!
 * @file command_encoder.h
 * @brief Type-erased command encoder for recording GPU commands.
 */

#ifndef RAKTR_RENDER_COMMAND_ENCODER_H
#define RAKTR_RENDER_COMMAND_ENCODER_H

#include "buffer.h"
#include "command_buffer.h"
#include "compute_pass_encoder.h"
#include "render_pass_encoder.h"
#include <memory>
#include <optional>
#include <string_view>

namespace raktr::render
{
    /*!
     * @brief Type-erased command encoder for recording GPU commands.
     *
     * CommandEncoder records GPU operations (draws, dispatches, copies) into
     * a CommandBuffer that can be submitted to a Queue for execution.
     *
     * Commands are recorded in order and executed sequentially when submitted.
     *
     * @example
     * auto encoder = device.create_command_encoder("My Encoder");
     *
     * // TODO: Record render pass
     * // auto pass = encoder.begin_render_pass(...);
     * // pass.draw(...);
     * // pass.end();
     *
     * // Copy between buffers
     * encoder.copy_buffer_to_buffer(src_buffer, 0, dst_buffer, 0, 1024);
     *
     * // Finish recording
     * CommandBuffer commands = encoder.finish();
     * device.queue().submit({commands});
     */
    class CommandEncoder
    {
    public:
        /*!
         * @brief Construct a CommandEncoder from any concrete encoder type.
         * @param encoder_impl Concrete encoder instance (WgpuCommandEncoder, etc.).
         */
        template <typename T>
        CommandEncoder(T encoder_impl)
            : _impl(std::make_unique<Model<T>>(std::move(encoder_impl)))
        {
        }

        /*!
         * @brief Construct a CommandEncoder from a pointer (non-owning).
         * @param encoder_ptr Pointer to existing encoder. Caller retains ownership.
         * @warning The pointed-to encoder must outlive this CommandEncoder instance.
         */
        template <typename T>
        CommandEncoder(T* encoder_ptr)
            : _impl(std::make_unique<Model<T>>(encoder_ptr))
        {
        }

        // Non-copyable
        CommandEncoder(const CommandEncoder&)            = delete;
        CommandEncoder& operator=(const CommandEncoder&) = delete;

        // Movable
        CommandEncoder(CommandEncoder&&) noexcept            = default;
        CommandEncoder& operator=(CommandEncoder&&) noexcept = default;

        ~CommandEncoder() = default;

        /*!<line_break>         * @brief Begin a render pass for recording graphics commands.
         * @param descriptor Render pass descriptor specifying attachments and load/store operations.
         * @return RenderPassEncoder for recording draw calls.
         *
         * @example
         * RenderPassDescriptor desc;
         * desc.color_attachments[0].view = surface_view;
         * desc.color_attachments[0].load_op = LoadOp::Clear;
         * desc.color_attachments[0].clear_color = {0.1, 0.2, 0.3, 1.0};
         * desc.color_attachment_count = 1;
         *
         * auto pass = encoder.begin_render_pass(desc);
         * pass.set_vertex_buffer(0, vertex_buffer);
         * pass.draw(3, 1, 0, 0);
         * pass.end();
         */
        [[nodiscard]] RenderPassEncoder begin_render_pass(const RenderPassDescriptor& descriptor) const
        {
            return _impl->do_begin_render_pass(descriptor);
        }

        /*!<line_break>         * @brief Begin a compute pass for recording compute commands.
         * @param descriptor Compute pass descriptor.
         * @return ComputePassEncoder for recording dispatches.
         *
         * @example
         * auto pass = encoder.begin_compute_pass({});
         * pass.dispatch(workgroup_x, workgroup_y, workgroup_z);
         * pass.end();
         */
        [[nodiscard]] ComputePassEncoder begin_compute_pass(const ComputePassDescriptor& descriptor) const
        {
            return _impl->do_begin_compute_pass(descriptor);
        }

        /*!
         * @brief Copy data between GPU buffers.
         * @param source Source buffer to copy from.
         * @param source_offset Byte offset in source buffer.
         * @param destination Destination buffer to copy to.
         * @param destination_offset Byte offset in destination buffer.
         * @param size Number of bytes to copy.
         *
         * Both buffers must have appropriate usage flags:
         * - Source: CopySrc
         * - Destination: CopyDst
         *
         * @example
         * encoder.copy_buffer_to_buffer(staging_buffer, 0, gpu_buffer, 0, 1024);
         */
        void copy_buffer_to_buffer(Buffer source, uint64_t source_offset, Buffer destination, uint64_t destination_offset, uint64_t size) const
        {
            _impl->do_copy_buffer_to_buffer(source, source_offset, destination, destination_offset, size);
        }

        /*!
         * @brief Finish recording and create a CommandBuffer ready for submission.
         * @return Immutable CommandBuffer containing recorded commands.
         *
         * After calling finish(), this encoder cannot be used anymore.
         * Create a new encoder for recording additional commands.
         *
         * @example
         * auto encoder = device.create_command_encoder();
         * encoder.copy_buffer_to_buffer(src, 0, dst, 0, 256);
         * CommandBuffer commands = encoder.finish();
         * device.queue().submit({commands});
         */
        [[nodiscard]] CommandBuffer finish() const
        {
            return _impl->do_finish();
        }

    private:
        /*!
         * @brief Internal interface for command encoder operations.
         */
        struct Concept
        {
            virtual ~Concept()                                                                                                                                               = default;
            virtual RenderPassEncoder  do_begin_render_pass(const RenderPassDescriptor& descriptor) const                                                                    = 0;
            virtual ComputePassEncoder do_begin_compute_pass(const ComputePassDescriptor& descriptor) const                                                                  = 0;
            virtual void               do_copy_buffer_to_buffer(Buffer source, uint64_t source_offset, Buffer destination, uint64_t destination_offset, uint64_t size) const = 0;
            virtual CommandBuffer      do_finish() const                                                                                                                     = 0;
        };

        /*!
         * @brief Model implementation wrapping concrete encoder type T.
         */
        template <typename T>
        struct Model : Concept
        {
            // Owning constructor
            explicit Model(T encoder_impl)
                : _encoder_storage(std::move(encoder_impl)), _encoder_ptr(&*_encoder_storage), _owns(true)
            {
            }

            // Non-owning constructor
            explicit Model(T* encoder_ptr)
                : _encoder_storage(std::nullopt), _encoder_ptr(encoder_ptr), _owns(false)
            {
            }

            RenderPassEncoder do_begin_render_pass(const RenderPassDescriptor& descriptor) const override
            {
                return _encoder_ptr->begin_render_pass(descriptor);
            }

            ComputePassEncoder do_begin_compute_pass(const ComputePassDescriptor& descriptor) const override
            {
                return _encoder_ptr->begin_compute_pass(descriptor);
            }

            void do_copy_buffer_to_buffer(Buffer source, uint64_t source_offset, Buffer destination, uint64_t destination_offset, uint64_t size) const override
            {
                _encoder_ptr->copy_buffer_to_buffer(source, source_offset, destination, destination_offset, size);
            }

            CommandBuffer do_finish() const override
            {
                return _encoder_ptr->finish();
            }

        private:
            std::optional<T> _encoder_storage; // Used if _owns == true
            T*               _encoder_ptr;     // Points to storage or external
            bool             _owns;            // Track ownership
        };

        std::unique_ptr<Concept> _impl;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_COMMAND_ENCODER_H
