/*!
 * @file shader_module.h
 * @brief Type-erased shader module for GPU shader compilation.
 *
 * Provides a platform-agnostic API for creating and managing shader modules
 * from WGSL source code. Follows the Concept-Model-Object pattern for type erasure.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace raktr::render
{

    /*!
     * @brief Shader module descriptor.
     *
     * Describes the shader source code and entry point for compilation.
     */
    struct ShaderModuleDescriptor
    {
        std::string_view code;       //!< WGSL shader source code
        std::string_view label = ""; //!< Optional debug label
    };

    /*!
     * @brief Type-erased shader module.
     *
     * Represents a compiled shader module that can be used in pipeline creation.
     * Uses Concept-Model-Object pattern for backend abstraction.
     *
     * @example
     * @code
     * ShaderModuleDescriptor desc;
     * desc.code = R"(
     *     @vertex
     *     fn vs_main(@location(0) position: vec3f) -> @builtin(position) vec4f {
     *         return vec4f(position, 1.0);
     *     }
     * )";
     * desc.label = "VertexShader";
     * auto shader = device.create_shader_module(desc);
     * @endcode
     */
    class ShaderModule
    {
    public:
        /*!
         * @brief Default constructor - creates invalid shader module.
         */
        ShaderModule() : _self(nullptr), _ptr(nullptr), _owns(false)
        {
        }

        /*!
         * @brief Construct from backend implementation.
         * @tparam T Backend shader module type (WgpuShaderModule, etc.)
         * @param impl Backend implementation instance
         */
        template <typename T>
        explicit ShaderModule(T impl)
            : _self(std::make_unique<Model<T>>(std::move(impl))),
              _ptr(_self.get()),
              _owns(true)
        {
        }

        /*!
         * @brief Copy constructor.
         */
        ShaderModule(const ShaderModule& other)
            : _self(other._self ? other._self->clone() : nullptr),
              _ptr(_self.get()),
              _owns(true)
        {
        }

        /*!
         * @brief Move constructor.
         */
        ShaderModule(ShaderModule&& other) noexcept
            : _self(std::move(other._self)),
              _ptr(other._ptr),
              _owns(other._owns)
        {
            other._ptr  = nullptr;
            other._owns = false;
        }

        /*!
         * @brief Copy assignment.
         */
        ShaderModule& operator=(const ShaderModule& other)
        {
            if (this != &other)
            {
                _self = other._self ? other._self->clone() : nullptr;
                _ptr  = _self.get();
                _owns = true;
            }
            return *this;
        }

        /*!
         * @brief Move assignment.
         */
        ShaderModule& operator=(ShaderModule&& other) noexcept
        {
            if (this != &other)
            {
                _self       = std::move(other._self);
                _ptr        = other._ptr;
                _owns       = other._owns;
                other._ptr  = nullptr;
                other._owns = false;
            }
            return *this;
        }

        /*!
         * @brief Check if shader module is valid.
         */
        [[nodiscard]] bool is_valid() const
        {
            return _ptr != nullptr;
        }

        /*!
         * @brief Explicit bool conversion.
         */
        explicit operator bool() const
        {
            return is_valid();
        }

        /*!
         * @brief Get backend-specific handle.
         * @return Opaque pointer to backend shader module
         */
        [[nodiscard]] void* native_handle() const
        {
            return _ptr ? _ptr->native_handle() : nullptr;
        }

    private:
        /*!
         * @brief Type erasure concept interface.
         */
        struct Concept
        {
            virtual ~Concept()                                     = default;
            virtual std::unique_ptr<Concept> clone() const         = 0;
            virtual void*                    native_handle() const = 0;
        };

        /*!
         * @brief Type erasure model implementation.
         */
        template <typename T>
        struct Model final : Concept
        {
            explicit Model(T impl) : _impl(std::move(impl))
            {
            }

            std::unique_ptr<Concept> clone() const override
            {
                return std::make_unique<Model<T>>(_impl);
            }

            void* native_handle() const override
            {
                if constexpr (requires { _impl.native_handle(); })
                {
                    return _impl.native_handle();
                }
                return nullptr;
            }

            T _impl;
        };

        std::unique_ptr<Concept> _self; //!< Owning pointer to implementation
        Concept*                 _ptr;  //!< Non-owning pointer for fast access
        bool                     _owns; //!< Whether this instance owns the implementation
    };

} // namespace raktr::render
