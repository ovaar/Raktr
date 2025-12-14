/*!
 * @file command_buffer.h
 * @brief Opaque handle to a recorded command buffer ready for submission.
 */

#ifndef RAKTR_RENDER_COMMAND_BUFFER_H
#define RAKTR_RENDER_COMMAND_BUFFER_H

#include <cstdint>

namespace raktr::render
{
    /*!
     * @brief Opaque handle to a GPU command buffer.
     *
     * Command buffers contain recorded GPU commands (draw calls, compute dispatches, copies, etc.)
     * that are ready to be submitted to a Queue for execution.
     *
     * Created by CommandEncoder::finish().
     * Submitted via Queue::submit().
     *
     * @example
     * auto encoder = device.create_command_encoder();
     * // ... record commands
     * CommandBuffer commands = encoder.finish();
     * device.queue().submit({commands});
     */
    class CommandBuffer
    {
    public:
        CommandBuffer() = default;

        /*!
         * @brief Construct from backend-specific handle.
         * @param id Backend-specific command buffer identifier.
         */
        explicit CommandBuffer(uint64_t id)
            : _id(id)
        {
        }

        /*!
         * @brief Check if this is a valid command buffer.
         * @return True if the command buffer has been created.
         */
        [[nodiscard]] bool is_valid() const
        {
            return _id != 0;
        }

        /*!
         * @brief Get the backend-specific identifier.
         * @return Command buffer ID.
         */
        [[nodiscard]] uint64_t id() const
        {
            return _id;
        }

    private:
        uint64_t _id = 0;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_COMMAND_BUFFER_H
