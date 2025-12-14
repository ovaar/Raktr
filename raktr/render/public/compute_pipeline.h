/*!
 * @file compute_pipeline.h
 * @brief Type-erased compute pipeline for compute shader execution.
 *
 * Provides a platform-agnostic API for creating immutable compute pipeline state objects.
 */

#pragma once

#include "shader_module.h"
#include <cstdint>
#include <memory>
#include <string_view>

namespace raktr::render
{

    /*!
     * @brief Compute pipeline descriptor.
     *
     * Describes the compute shader and entry point for pipeline creation.
     */
    struct ComputePipelineDescriptor
    {
        std::string_view label = "";

        // Compute stage
        ShaderModule     compute_shader;
        std::string_view compute_entry_point = "cs_main";

        // Bind group layouts (Phase 3 - will be set later)
        // std::vector<BindGroupLayout> bind_group_layouts;
    };

    /*!
     * @brief Type-erased compute pipeline.
     *
     * Represents an immutable compute pipeline state object.
     * Once created, pipeline state cannot be changed.
     *
     * @example
     * @code
     * ComputePipelineDescriptor desc;
     * desc.compute_shader = compute_shader_module;
     * desc.compute_entry_point = "cs_main";
     * desc.label = "ParticleUpdate";
     * auto pipeline = device.create_compute_pipeline(desc);
     * @endcode
     */
    class ComputePipeline
    {
    public:
        ComputePipeline() : _self(nullptr), _ptr(nullptr), _owns(false)
        {
        }

        template <typename T>
        explicit ComputePipeline(T impl)
            : _self(std::make_unique<Model<T>>(std::move(impl))),
              _ptr(_self.get()),
              _owns(true)
        {
        }

        // Non-copyable (GPU resources cannot be cloned)
        ComputePipeline(const ComputePipeline&) = delete;

        ComputePipeline(ComputePipeline&& other) noexcept
            : _self(std::move(other._self)),
              _ptr(other._ptr),
              _owns(other._owns)
        {
            other._ptr  = nullptr;
            other._owns = false;
        }

        // Non-copyable (GPU resources cannot be cloned)
        ComputePipeline& operator=(const ComputePipeline&) = delete;

        ComputePipeline& operator=(ComputePipeline&& other) noexcept
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

} // namespace raktr::render
