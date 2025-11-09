/*!
 * @file device.h
 * @brief GPU device abstraction for creating resources.
 */

#ifndef RAKTR_RENDER_DEVICE_H
#define RAKTR_RENDER_DEVICE_H

#include <cstddef>
#include <expected>
#include <span>
#include <system_error>

namespace raktr::render
{
    class Buffer;
    enum class AspectRatio;
    struct Viewport;

    /*!
     * @brief GPU device abstraction for creating resources.
     */
    class Device
    {
    public:
        Device()          = default;
        virtual ~Device() = default;

        // Non-copyable, non-moveable (abstract interface)
        Device(const Device&)            = delete;
        Device& operator=(const Device&) = delete;

        /*!
         * @brief Create a vertex buffer.
         * @param data Vertex data to upload.
         * @return Buffer handle or error.
         */
        virtual std::expected<Buffer, std::error_code>
        create_vertex_buffer(std::span<const std::byte> data) = 0;

        /*!
         * @brief Create an index buffer.
         * @param data Index data to upload.
         * @return Buffer handle or error.
         */
        virtual std::expected<Buffer, std::error_code>
        create_index_buffer(std::span<const std::byte> data) = 0;

        /*!
         * @brief Submit a simple draw call (for MVP).
         * @param vertex_buffer Vertex buffer to bind.
         * @param index_buffer Index buffer to bind.
         * @param index_count Number of indices to draw.
         * @return Success or error.
         */
        virtual std::expected<void, std::error_code>
        draw_indexed(const Buffer& vertex_buffer,
                     const Buffer& index_buffer,
                     uint32_t      index_count) = 0;

        /*!
         * @brief Clear the current render target.
         */
        virtual void clear() = 0;

        /*!
         * @brief Present the rendered frame (swap buffers).
         */
        virtual void present() = 0;

        /*!
         * @brief Create a uniform buffer for shader constants.
         * @param size Size of the uniform buffer in bytes.
         * @return Buffer handle or error code.
         */
        [[nodiscard]] virtual std::expected<Buffer, std::error_code>
        create_uniform_buffer(size_t size) = 0;

        /*!
         * @brief Update uniform buffer data.
         * @param buffer Buffer handle from create_uniform_buffer().
         * @param data Data to upload.
         * @return Success or error code.
         */
        [[nodiscard]] virtual std::expected<void, std::error_code>
        update_uniform_buffer(const Buffer& buffer, std::span<const std::byte> data) = 0;

        /*!
         * @brief Set uniform buffer for rendering.
         * Must be called before draw_indexed() to bind uniforms.
         * @param buffer Uniform buffer to bind.
         */
        virtual void set_uniform_buffer(const Buffer& buffer) = 0;

        /*!
         * @brief Resize the surface to new dimensions.
         *
         * Reconfigures the WebGPU surface with new width and height.
         * Should be called when the window is resized.
         *
         * @param width New width in pixels (must be > 0).
         * @param height New height in pixels (must be > 0).
         * @return Success or error code.
         *
         * @example
         * // In window resize callback:
         * window->set_resize_callback([&device](uint32_t w, uint32_t h) {
         *     device->resize(w, h);
         * });
         */
        [[nodiscard]] virtual std::expected<void, std::error_code>
        resize(uint32_t width, uint32_t height) = 0;

        /*!
         * @brief Set the aspect ratio for rendering.
         *
         * Controls how the viewport maintains proportions when the window is resized.
         * Uses letterboxing (black bars top/bottom) or pillarboxing (black bars left/right)
         * to maintain the specified aspect ratio.
         *
         * @param ratio Desired aspect ratio (default: Ratio_16_9).
         * @param custom_value Custom ratio value (only used if ratio == Custom).
         *
         * @example
         * // Use 16:9 aspect ratio (most common)
         * device->set_aspect_ratio(AspectRatio::Ratio_16_9);
         *
         * // Use ultrawide 21:9
         * device->set_aspect_ratio(AspectRatio::Ratio_21_9);
         *
         * // Use custom cinema ratio
         * device->set_aspect_ratio(AspectRatio::Custom, 2.35f);
         *
         * // Allow free stretching (no constraint)
         * device->set_aspect_ratio(AspectRatio::Auto);
         */
        virtual void set_aspect_ratio(AspectRatio ratio, float custom_value = 1.0f) = 0;

        /*!
         * @brief Get the current aspect ratio setting.
         * @return Current aspect ratio mode.
         */
        [[nodiscard]] virtual AspectRatio aspect_ratio() const = 0;

        /*!
         * @brief Get the current viewport rectangle.
         *
         * Returns the viewport used for rendering, which may be smaller than
         * the window if aspect ratio preservation is enabled.
         *
         * @return Current viewport (x, y, width, height).
         */
        [[nodiscard]] virtual const Viewport& viewport() const = 0;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_DEVICE_H
