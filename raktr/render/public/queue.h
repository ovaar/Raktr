/*!
 * @file queue.h
 * @brief Type-erased GPU queue abstraction for command submission.
 */

#ifndef RAKTR_RENDER_QUEUE_H
#define RAKTR_RENDER_QUEUE_H

#include "buffer.h"
#include "command_buffer.h"
#include <memory>
#include <optional>
#include <span>

namespace raktr::render
{
    /*!
     * @brief Type-erased GPU queue for submitting commands and transferring data.
     *
     * The Queue is the interface between CPU and GPU. It handles:
     * - Submitting command buffers for execution
     * - Writing data directly to GPU buffers
     * - Synchronization (future: fences, timestamps)
     *
     * @example
     * // Submit commands
     * auto encoder = device.create_command_encoder();
     * // ... record commands
     * auto commands = encoder.finish();
     * device.queue().submit({commands});
     *
     * // Write data directly to buffer
     * std::vector<float> data = {1.0f, 2.0f, 3.0f};
     * device.queue().write_buffer(buffer, 0, std::as_bytes(std::span(data)));
     */
    class Queue
    {
    public:
        /*!
         * @brief Construct a Queue from any concrete queue type.
         * @param queue_impl Concrete queue instance (WgpuQueue, etc.).
         */
        template <typename T>
        Queue(T queue_impl)
            : _impl(std::make_unique<Model<T>>(std::move(queue_impl)))
        {
        }

        /*!
         * @brief Construct a Queue from a pointer (non-owning).
         * @param queue_ptr Pointer to existing queue. Caller retains ownership.
         * @warning The pointed-to queue must outlive this Queue instance.
         */
        template <typename T>
        Queue(T* queue_ptr)
            : _impl(std::make_unique<Model<T>>(queue_ptr))
        {
        }

        // Non-copyable
        Queue(const Queue&)            = delete;
        Queue& operator=(const Queue&) = delete;

        // Movable
        Queue(Queue&&) noexcept            = default;
        Queue& operator=(Queue&&) noexcept = default;

        ~Queue() = default;

        /*!
         * @brief Submit command buffers to the GPU for execution.
         * @param commands Span of command buffers to execute in order.
         *
         * Commands are executed asynchronously. Use fences or sync primitives
         * to wait for completion.
         *
         * @example
         * CommandBuffer cmd1 = encoder1.finish();
         * CommandBuffer cmd2 = encoder2.finish();
         * queue.submit({cmd1, cmd2}); // Execute cmd1, then cmd2
         */
        void submit(std::span<const CommandBuffer> commands) const
        {
            _impl->do_submit(commands);
        }

        /*!
         * @brief Write data directly to a GPU buffer.
         * @param buffer Target buffer to write to.
         * @param offset Byte offset into the buffer.
         * @param data Data to write.
         *
         * This is a convenience method for small data transfers. For large
         * transfers, consider using staging buffers and copy commands.
         *
         * @example
         * std::vector<float> vertices = {0.0f, 0.5f, 0.0f};
         * queue.write_buffer(vertex_buffer, 0, std::as_bytes(std::span(vertices)));
         */
        void write_buffer(Buffer buffer, uint64_t offset, std::span<const std::byte> data) const
        {
            _impl->do_write_buffer(buffer, offset, data);
        }

    private:
        /*!
         * @brief Internal interface for queue operations.
         */
        struct Concept
        {
            virtual ~Concept()                                                                                  = default;
            virtual void do_submit(std::span<const CommandBuffer> commands) const                               = 0;
            virtual void do_write_buffer(Buffer buffer, uint64_t offset, std::span<const std::byte> data) const = 0;
        };

        /*!
         * @brief Model implementation wrapping concrete queue type T.
         */
        template <typename T>
        struct Model : Concept
        {
            // Owning constructor
            explicit Model(T queue_impl)
                : _queue_storage(std::move(queue_impl)), _queue_ptr(&*_queue_storage), _owns(true)
            {
            }

            // Non-owning constructor
            explicit Model(T* queue_ptr)
                : _queue_storage(std::nullopt), _queue_ptr(queue_ptr), _owns(false)
            {
            }

            void do_submit(std::span<const CommandBuffer> commands) const override
            {
                _queue_ptr->submit(commands);
            }

            void do_write_buffer(Buffer buffer, uint64_t offset, std::span<const std::byte> data) const override
            {
                _queue_ptr->write_buffer(buffer, offset, data);
            }

        private:
            std::optional<T> _queue_storage; // Used if _owns == true
            T*               _queue_ptr;     // Points to storage or external
            bool             _owns;          // Track ownership
        };

        std::unique_ptr<Concept> _impl;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_QUEUE_H
