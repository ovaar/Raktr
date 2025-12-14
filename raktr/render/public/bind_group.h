/*!
 * @file bind_group.h
 * @brief Type-erased bind groups for resource binding in shaders.
 *
 * Provides a platform-agnostic API for creating bind group layouts and bind groups
 * that bind uniform buffers, storage buffers, textures, and samplers to shader stages.
 */

#pragma once

#include "buffer.h"
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace raktr::render
{

    /*!
     * @brief Shader stage visibility flags.
     */
    enum class ShaderStage : uint8_t
    {
        None     = 0x0, //!< No stages
        Vertex   = 0x1, //!< Vertex shader
        Fragment = 0x2, //!< Fragment shader
        Compute  = 0x4, //!< Compute shader
        All      = 0x7  //!< All stages
    };

    /*!
     * @brief Binding resource type.
     */
    enum class BindingType : uint8_t
    {
        UniformBuffer  = 0, //!< Uniform buffer (read-only)
        StorageBuffer  = 1, //!< Storage buffer (read-write)
        Texture        = 2, //!< Texture (sampled)
        Sampler        = 3, //!< Sampler state
        StorageTexture = 4  //!< Storage texture (read-write)
    };

    /*!
     * @brief Bind group layout entry.
     *
     * Describes a single binding slot in a bind group layout.
     */
    struct BindGroupLayoutEntry
    {
        uint32_t    binding;    //!< Binding slot number
        ShaderStage visibility; //!< Shader stages that can access this binding
        BindingType type;       //!< Type of resource bound to this slot
    };

    /*!
     * @brief Bind group layout descriptor.
     */
    struct BindGroupLayoutDescriptor
    {
        std::string_view                  label = "";
        std::vector<BindGroupLayoutEntry> entries;
    };

    /*!
     * @brief Type-erased bind group layout.
     *
     * Describes the structure and types of resources in a bind group.
     * Bind group layouts are used to create compatible bind groups and pipelines.
     */
    class BindGroupLayout
    {
    public:
        BindGroupLayout() : _self(nullptr), _ptr(nullptr), _owns(false)
        {
        }

        template <typename T>
        explicit BindGroupLayout(T impl)
            : _self(std::make_unique<Model<T>>(std::move(impl))),
              _ptr(_self.get()),
              _owns(true)
        {
        }

        // Non-copyable (GPU resources cannot be cloned)
        BindGroupLayout(const BindGroupLayout&)            = delete;
        BindGroupLayout& operator=(const BindGroupLayout&) = delete;

        // Movable
        BindGroupLayout(BindGroupLayout&& other) noexcept
            : _self(std::move(other._self)),
              _ptr(other._ptr),
              _owns(other._owns)
        {
            other._ptr  = nullptr;
            other._owns = false;
        }

        BindGroupLayout& operator=(BindGroupLayout&& other) noexcept
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

        [[nodiscard]] bool is_valid() const
        {
            return _ptr != nullptr;
        }
        explicit operator bool() const
        {
            return is_valid();
        }

        [[nodiscard]] void* native_handle() const
        {
            return _ptr ? _ptr->native_handle() : nullptr;
        }

    private:
        struct Concept
        {
            virtual ~Concept()                  = default;
            virtual void* native_handle() const = 0;
        };

        template <typename T>
        struct Model final : Concept
        {
            explicit Model(T impl) : _impl(std::move(impl))
            {
            }

            void* native_handle() const override
            {
                if constexpr (requires { _impl.native_handle(); })
                {
                    return _impl.native_handle();
                }
                else
                {
                    return nullptr;
                }
            }

            T _impl;
        };

        std::unique_ptr<Concept> _self;
        Concept*                 _ptr;
        bool                     _owns;
    };

    /*!
     * @brief Bind group entry.
     *
     * Binds a specific resource to a binding slot.
     */
    struct BindGroupEntry
    {
        uint32_t binding;             //!< Binding slot number (must match layout)
        Buffer   buffer;              //!< Buffer resource (for uniform/storage buffers)
        uint64_t offset = 0;          //!< Offset into buffer
        uint64_t size   = UINT64_MAX; //!< Size of binding (UINT64_MAX = whole buffer)

        // Note: Texture and sampler bindings will be added in future phases
        // void* texture = nullptr;
        // void* sampler = nullptr;
    };

    /*!
     * @brief Bind group descriptor.
     */
    struct BindGroupDescriptor
    {
        std::string_view            label = "";
        BindGroupLayout             layout;  //!< Layout that this bind group conforms to
        std::vector<BindGroupEntry> entries; //!< Resource bindings
    };

    /*!
     * @brief Type-erased bind group.
     *
     * Contains actual resource bindings for use in render/compute passes.
     * Bind groups are created from a bind group layout and can be bound to pipelines
     * using set_bind_group() on pass encoders.
     *
     * @example
     * @code
     * // Create layout
     * BindGroupLayoutDescriptor layout_desc;
     * layout_desc.entries.push_back({
     *     .binding = 0,
     *     .visibility = ShaderStage::Vertex,
     *     .type = BindingType::UniformBuffer
     * });
     * auto layout = device.create_bind_group_layout(layout_desc);
     *
     * // Create bind group
     * BindGroupDescriptor bind_desc;
     * bind_desc.layout = layout;
     * bind_desc.entries.push_back({
     *     .binding = 0,
     *     .buffer = uniform_buffer
     * });
     * auto bind_group = device.create_bind_group(bind_desc);
     *
     * // Use in render pass
     * render_pass.set_bind_group(0, bind_group);
     * @endcode
     */
    class BindGroup
    {
    public:
        BindGroup() : _self(nullptr), _ptr(nullptr), _owns(false)
        {
        }

        template <typename T>
        explicit BindGroup(T impl)
            : _self(std::make_unique<Model<T>>(std::move(impl))),
              _ptr(_self.get()),
              _owns(true)
        {
        }

        // Non-copyable (GPU resources cannot be cloned)
        BindGroup(const BindGroup&)            = delete;
        BindGroup& operator=(const BindGroup&) = delete;

        // Movable
        BindGroup(BindGroup&& other) noexcept
            : _self(std::move(other._self)),
              _ptr(other._ptr),
              _owns(other._owns)
        {
            other._ptr  = nullptr;
            other._owns = false;
        }

        BindGroup& operator=(BindGroup&& other) noexcept
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

        [[nodiscard]] bool is_valid() const
        {
            return _ptr != nullptr;
        }
        explicit operator bool() const
        {
            return is_valid();
        }

        [[nodiscard]] void* native_handle() const
        {
            return _ptr ? _ptr->native_handle() : nullptr;
        }

    private:
        struct Concept
        {
            virtual ~Concept()                  = default;
            virtual void* native_handle() const = 0;
        };

        template <typename T>
        struct Model final : Concept
        {
            explicit Model(T impl) : _impl(std::move(impl))
            {
            }

            void* native_handle() const override
            {
                if constexpr (requires { _impl.native_handle(); })
                {
                    return _impl.native_handle();
                }
                else
                {
                    return nullptr;
                }
            }

            T _impl;
        };

        std::unique_ptr<Concept> _self;
        Concept*                 _ptr;
        bool                     _owns;
    };

    /*!
     * @brief Bitwise OR for ShaderStage flags.
     */
    inline ShaderStage operator|(ShaderStage lhs, ShaderStage rhs)
    {
        return static_cast<ShaderStage>(
            static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }

    /*!
     * @brief Bitwise AND for ShaderStage flags.
     */
    inline ShaderStage operator&(ShaderStage lhs, ShaderStage rhs)
    {
        return static_cast<ShaderStage>(
            static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
    }

} // namespace raktr::render
