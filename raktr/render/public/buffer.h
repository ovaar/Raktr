/*!
 * @file buffer.h
 * @brief GPU buffer handle abstraction.
 */

#ifndef RAKTR_RENDER_BUFFER_H
#define RAKTR_RENDER_BUFFER_H

#include <cstdint>

namespace raktr::render
{
    /*!
     * @brief Type of GPU buffer.
     */
    enum class BufferType
    {
        Vertex,
        Index
    };

    /*!
     * @brief Handle to a GPU buffer resource.
     * 
     * This is a lightweight handle that can be copied.
     * The underlying resource is managed by the backend.
     */
    class Buffer
    {
    public:
        /*!
         * @brief Construct an invalid buffer handle.
         */
        Buffer() : _id(0), _type(BufferType::Vertex) {}

        /*!
         * @brief Construct a buffer handle.
         * @param id Backend-specific buffer identifier.
         * @param type Type of buffer (vertex or index).
         */
        Buffer(uint64_t id, BufferType type) 
            : _id(id), _type(type) {}

        /*!
         * @brief Check if buffer handle is valid.
         */
        bool is_valid() const { return _id != 0; }

        /*!
         * @brief Get the buffer identifier.
         */
        uint64_t id() const { return _id; }

        /*!
         * @brief Get the buffer type.
         */
        BufferType type() const { return _type; }

    private:
        uint64_t _id;
        BufferType _type;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_BUFFER_H
