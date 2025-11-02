/*!
 * @file render_error.h
 * @brief Error codes and error handling for the Raktr render subsystem.
 */

#ifndef RAKTR_RENDER_RENDER_ERROR_H
#define RAKTR_RENDER_RENDER_ERROR_H

#include <system_error>
#include <string>

namespace raktr::render
{
    /*!
     * @brief Render subsystem error codes.
     */
    enum class RenderError
    {
        Success = 0,
        BackendNotSupported,
        InitializationFailed,
        DeviceCreationFailed,
        BufferCreationFailed,
        ShaderCompilationFailed,
        InvalidOperation,
        WindowCreationFailed
    };

    /*!
     * @brief Custom error category for render errors.
     */
    class RenderErrorCategory : public std::error_category
    {
    public:
        const char* name() const noexcept override { return "raktr::render"; }

        std::string message(int ev) const override
        {
            switch (static_cast<RenderError>(ev))
            {
                case RenderError::Success: return "Success";
                case RenderError::BackendNotSupported: return "Backend not supported";
                case RenderError::InitializationFailed: return "Initialization failed";
                case RenderError::DeviceCreationFailed: return "Device creation failed";
                case RenderError::BufferCreationFailed: return "Buffer creation failed";
                case RenderError::ShaderCompilationFailed: return "Shader compilation failed";
                case RenderError::InvalidOperation: return "Invalid operation";
                case RenderError::WindowCreationFailed: return "Window creation failed";
                default: return "Unknown error";
            }
        }
    };

    /*!
     * @brief Get the global render error category.
     */
    inline const RenderErrorCategory& render_category()
    {
        static RenderErrorCategory category;
        return category;
    }

    /*!
     * @brief Create an error_code from a RenderError.
     */
    inline std::error_code make_error_code(RenderError e)
    {
        return {static_cast<int>(e), render_category()};
    }

} // namespace raktr::render

// Enable automatic conversion to std::error_code
namespace std
{
    template<>
    struct is_error_code_enum<raktr::render::RenderError> : true_type {};
}

#endif // RAKTR_RENDER_RENDER_ERROR_H
