/*!
 * @file render_context.h
 * @brief Main rendering context managing backend and resources.
 */

#ifndef RAKTR_RENDER_RENDER_CONTEXT_H
#define RAKTR_RENDER_RENDER_CONTEXT_H

#include "device.h"
#include "render_error.h"
#include <memory>
#include <expected>
#include <system_error>

namespace raktr::render
{
    /*!
     * @brief Supported graphics API backends.
     */
    enum class BackendType
    {
        Mock,        // For testing without actual GPU
        OpenGL,
        Vulkan,
        DirectX12
    };

    /*!
     * @brief Configuration for initializing a render context.
     */
    struct RenderConfig
    {
        BackendType backend = BackendType::Mock;
        bool enable_validation = false;  // Debug layers/validation
        bool enable_vsync = true;
    };

    /*!
     * @brief Main rendering context managing backend and resources.
     */
    class RenderContext
    {
    public:
        RenderContext();
        ~RenderContext();

        // Non-copyable, moveable
        RenderContext(const RenderContext&) = delete;
        RenderContext& operator=(const RenderContext&) = delete;
        RenderContext(RenderContext&&) noexcept;
        RenderContext& operator=(RenderContext&&) noexcept;

        /*!
         * @brief Initialize the render context with given configuration.
         * @param config Rendering configuration.
         * @return Success or error code.
         */
        std::expected<void, std::error_code> initialize(const RenderConfig& config);

        /*!
         * @brief Shutdown the render context and free resources.
         */
        void shutdown();

        /*!
         * @brief Get the initialized device.
         * @return Pointer to the device, or nullptr if not initialized.
         */
        Device* device() const;

        /*!
         * @brief Check if context is successfully initialized.
         */
        bool is_initialized() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    /*!
     * @brief Factory function to create a render context.
     * @return Unique pointer to RenderContext.
     */
    std::unique_ptr<RenderContext> create_render_context();

} // namespace raktr::render

#endif // RAKTR_RENDER_RENDER_CONTEXT_H
