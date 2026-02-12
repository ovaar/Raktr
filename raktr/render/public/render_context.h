/*!
 * @file render_context.h
 * @brief Main rendering context managing backend and resources.
 */

#ifndef RAKTR_RENDER_RENDER_CONTEXT_H
#define RAKTR_RENDER_RENDER_CONTEXT_H

#include "device.h"
#include "render_error.h"
#include <expected>
#include <memory>
#include <system_error>

namespace raktr::render
{
    class Window;        // Forward declaration
    struct WindowConfig; // Forward declaration

    /*!
     * @brief Supported graphics API backends.
     */
    enum class BackendType
    {
        Soft,   // Software renderer for testing
        WebGPU, // WebGPU via wgpu-native
        OpenGL,
        Vulkan,
        DirectX12
    };

    /*!
     * @brief Configuration for initializing a render context.
     */
    struct RenderConfig
    {
        BackendType backend           = BackendType::Soft;
        bool        enable_validation = false; // Debug layers/validation
        bool        enable_vsync      = true;
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
        RenderContext(const RenderContext&)            = delete;
        RenderContext& operator=(const RenderContext&) = delete;
        RenderContext(RenderContext&&) noexcept;
        RenderContext& operator=(RenderContext&&) noexcept;

        /*!
         * @brief Initialize with default window (backend owns window).
         * @param config Rendering configuration.
         * @return Success or error code.
         */
        [[nodiscard]] std::expected<void, std::error_code> initialize(const RenderConfig& config);

        /*!
         * @brief Initialize with custom window config (backend owns window).
         * @param config Rendering configuration.
         * @param window_config Window creation parameters.
         * @return Success or error code.
         */
        [[nodiscard]] std::expected<void, std::error_code> initialize(
            const RenderConfig& config,
            const WindowConfig& window_config);

        /*!
         * @brief Initialize with existing window (caller owns window).
         * @param config Rendering configuration.
         * @param window Externally-owned window (must outlive RenderContext).
         * @return Success or error code.
         */
        [[nodiscard]] std::expected<void, std::error_code> initialize(
            const RenderConfig& config,
            Window*             window);

        /*!
         * @brief Shutdown the render context and free resources.
         */
        void shutdown();

        /*!
         * @brief Get the initialized device.
         * @return Device view (check for validity with operator bool).
         */
        [[nodiscard]] DeviceView device() const;

        /*!
         * @brief Check if context is successfully initialized.
         */
        [[nodiscard]] bool is_initialized() const;

        /*!
         * @brief Get pointer to backend implementation (for backend-specific operations).
         * @return Pointer to backend, or nullptr if not initialized.
         * @note Use with caution - breaks abstraction. Only needed for advanced scenarios
         *       like RenderGraph PassContext setup with backend-specific resources.
         */
        [[nodiscard]] void* backend_ptr() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    /*!
     * @brief Factory function to create a render context.
     * @return Unique pointer to RenderContext.
     */
    [[nodiscard]] std::unique_ptr<RenderContext> create_render_context();

} // namespace raktr::render

#endif // RAKTR_RENDER_RENDER_CONTEXT_H
