/*!
 * @file device.h
 * @brief Type-erased GPU device abstraction using external polymorphism.
 *
 * This implementation uses Klaus Iglberger's type erasure pattern to provide
 * runtime polymorphism without inheritance. Devices can support different
 * subsets of capabilities, which can be queried at runtime.
 */

#ifndef RAKTR_RENDER_DEVICE_H
#define RAKTR_RENDER_DEVICE_H

#include "aspect_ratio.h"
#include "buffer.h"
#include "command_encoder.h"
#include "device_capabilities.h"
#include "queue.h"
#include "render_pipeline.h"
#include "shader_module.h"
#include <any>
#include <memory>
#include <optional>
#include <typeindex>
#include <unordered_map>

namespace raktr::render
{
    /*!
     * @brief Type-erased GPU device wrapper with capability-based interface.
     *
     * This class wraps any concrete device type (WgpuDevice, SoftDevice, etc.)
     * and provides capability-based access to device operations. Use supports<T>()
     * to check if a capability exists, then capability<T>() to access it.
     *
     * @example
     * Device device = create_wgpu_device(window);
     *
     * if (device.supports<capabilities::BufferOps>()) {
     *     auto& buffers = device.capability<capabilities::BufferOps>();
     *     auto vb = buffers.create_vertex_buffer(vertex_data);
     * }
     *
     * if (device.supports<capabilities::DrawOps>()) {
     *     auto& draw = device.capability<capabilities::DrawOps>();
     *     draw.clear();
     *     draw.draw_indexed(vb, ib, 36);
     * }
     */
    class Device
    {
    public:
        /*!
         * @brief Construct a Device from any concrete device type (owning).
         * @param device_impl Concrete device instance (WgpuDevice, SoftDevice, etc.).
         */
        template <typename T>
        Device(T device_impl)
            : _impl(std::make_unique<Model<T>>(std::move(device_impl)))
        {
        }

        /*!
         * @brief Construct a Device from a pointer (non-owning).
         * @param device_ptr Pointer to existing device. Caller retains ownership.
         * @warning The pointed-to device must outlive this Device instance.
         */
        template <typename T>
        Device(T* device_ptr)
            : _impl(std::make_unique<Model<T>>(device_ptr))
        {
        }

        // Non-copyable (some devices like WgpuDevice hold non-copyable resources)
        Device(const Device&)            = delete;
        Device& operator=(const Device&) = delete;

        // Movable
        Device(Device&&) noexcept            = default;
        Device& operator=(Device&&) noexcept = default;

        ~Device() = default;

        /*!
         * @brief Check if device supports a specific capability.
         * @tparam Capability Capability type (e.g., capabilities::BufferOps).
         * @return True if the device supports this capability.
         */
        template <typename Capability>
        [[nodiscard]] bool supports() const
        {
            return _impl && _impl->supports(std::type_index(typeid(Capability)));
        }

        /*!
         * @brief Get a capability interface from the device.
         * @tparam Capability Capability type to retrieve.
         * @return Capability interface by value.
         * @throws std::bad_optional_access if capability is not supported.
         */
        template <typename Capability>
        [[nodiscard]] Capability capability() const
        {
            if (!_impl)
            {
                throw std::runtime_error("Device not initialized");
            }

            auto result = _impl->capability<Capability>(std::type_index(typeid(Capability)));
            if (!result)
            {
                throw std::runtime_error("Capability not supported by this device");
            }
            return *std::move(result);
        }

        // Convenience methods that forward to capabilities (for backward compatibility)
        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_vertex_buffer(std::span<const std::byte> data) const
        {
            return capability<capabilities::BufferOps>().create_vertex_buffer(data);
        }

        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_index_buffer(std::span<const std::byte> data) const
        {
            return capability<capabilities::BufferOps>().create_index_buffer(data);
        }

        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_uniform_buffer(size_t size) const
        {
            return capability<capabilities::BufferOps>().create_uniform_buffer(size);
        }

        [[nodiscard]] std::expected<void, std::error_code>
        update_uniform_buffer(const Buffer& buffer, std::span<const std::byte> data) const
        {
            return capability<capabilities::BufferOps>().update_uniform_buffer(buffer, data);
        }

        void set_uniform_buffer(const Buffer& buffer) const
        {
            capability<capabilities::BufferOps>().set_uniform_buffer(buffer);
        }

        [[nodiscard]] std::expected<void, std::error_code>
        draw_indexed(const Buffer& vertex_buffer, const Buffer& index_buffer, uint32_t index_count) const
        {
            return capability<capabilities::DrawOps>().draw_indexed(vertex_buffer, index_buffer, index_count);
        }

        void clear() const
        {
            capability<capabilities::DrawOps>().clear();
        }

        void present() const
        {
            capability<capabilities::PresentOps>().present();
        }

        [[nodiscard]] Queue queue() const
        {
            return capability<capabilities::QueueOps>().queue();
        }

        [[nodiscard]] CommandEncoder create_command_encoder(std::string_view label = "") const
        {
            return capability<capabilities::CommandEncoderOps>().create_command_encoder(label);
        }

        [[nodiscard]] void* get_surface_view() const
        {
            return capability<capabilities::ViewportOps>().get_surface_view();
        }

        [[nodiscard]] void* get_depth_view() const
        {
            return capability<capabilities::ViewportOps>().get_depth_view();
        }

        [[nodiscard]] std::expected<void, std::error_code>
        resize(uint32_t width, uint32_t height) const
        {
            return capability<capabilities::ViewportOps>().resize(width, height);
        }

        void set_aspect_ratio(AspectRatio ratio, float custom_value = 1.0f) const
        {
            capability<capabilities::ViewportOps>().set_aspect_ratio(ratio, custom_value);
        }

        [[nodiscard]] AspectRatio aspect_ratio() const
        {
            return capability<capabilities::ViewportOps>().aspect_ratio();
        }

        [[nodiscard]] const Viewport& viewport() const
        {
            return capability<capabilities::ViewportOps>().viewport();
        }

        // Instancing operations (if supported)
        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_instance_buffer(std::span<const std::byte> data) const
        {
            return capability<capabilities::InstancingOps>().create_instance_buffer(data);
        }

        [[nodiscard]] std::expected<void, std::error_code>
        update_instance_buffer(const Buffer& buffer, std::span<const std::byte> data) const
        {
            return capability<capabilities::InstancingOps>().update_instance_buffer(buffer, data);
        }

        [[nodiscard]] std::expected<void, std::error_code>
        draw_indexed_instanced(const Buffer& vertex_buffer,
                               const Buffer& index_buffer,
                               const Buffer& instance_buffer,
                               uint32_t      index_count,
                               uint32_t      instance_count) const
        {
            return capability<capabilities::InstancingOps>().draw_indexed_instanced(
                vertex_buffer, index_buffer, instance_buffer, index_count, instance_count);
        }

        // Occlusion culling operations (if supported)
        [[nodiscard]] void* get_depth_texture() const
        {
            return capability<capabilities::OcclusionCullingOps>().get_depth_texture();
        }

        // Shader operations (if supported)
        [[nodiscard]] std::expected<ShaderModule, std::error_code>
        create_shader_module(const ShaderModuleDescriptor& descriptor) const
        {
            return capability<capabilities::ShaderOps>().create_shader_module(descriptor);
        }

        // Render pipeline operations (if supported)
        [[nodiscard]] std::expected<RenderPipeline, std::error_code>
        create_render_pipeline(const RenderPipelineDescriptor& descriptor) const
        {
            return capability<capabilities::RenderPipelineOps>().create_render_pipeline(descriptor);
        }

    private:
        /*!
         * @brief Concept interface for type-erased device implementation.
         */
        struct Concept
        {
            virtual ~Concept()                                                                             = default;
            [[nodiscard]] virtual bool                           supports(std::type_index ti) const        = 0;
            [[nodiscard]] virtual std::optional<std::type_index> capability_type(std::type_index ti) const = 0;

            template <typename Capability>
            std::optional<Capability> capability(std::type_index ti) const
            {
                return do_capability<Capability>(ti);
            }

        private:
            [[nodiscard]] virtual std::optional<capabilities::BufferOps>           do_capability_bufferops() const           = 0;
            [[nodiscard]] virtual std::optional<capabilities::DrawOps>             do_capability_drawops() const             = 0;
            [[nodiscard]] virtual std::optional<capabilities::ViewportOps>         do_capability_viewportops() const         = 0;
            [[nodiscard]] virtual std::optional<capabilities::PresentOps>          do_capability_presentops() const          = 0;
            [[nodiscard]] virtual std::optional<capabilities::InstancingOps>       do_capability_instancingops() const       = 0;
            [[nodiscard]] virtual std::optional<capabilities::OcclusionCullingOps> do_capability_occlusioncullingops() const = 0;
            [[nodiscard]] virtual std::optional<capabilities::QueueOps>            do_capability_queueops() const            = 0;
            [[nodiscard]] virtual std::optional<capabilities::CommandEncoderOps>   do_capability_commandencoderops() const   = 0;
            [[nodiscard]] virtual std::optional<capabilities::ShaderOps>           do_capability_shaderops() const           = 0;
            [[nodiscard]] virtual std::optional<capabilities::RenderPipelineOps>   do_capability_renderpipelineops() const   = 0;
            [[nodiscard]] virtual std::optional<capabilities::ComputePipelineOps>  do_capability_computepipelineops() const  = 0;
            [[nodiscard]] virtual std::optional<capabilities::BindGroupOps>        do_capability_bindgroupops() const        = 0;

            template <typename Capability>
            std::optional<Capability> do_capability(std::type_index /* ti */) const
            {
                if constexpr (std::is_same_v<Capability, capabilities::BufferOps>)
                {
                    return do_capability_bufferops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::DrawOps>)
                {
                    return do_capability_drawops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::ViewportOps>)
                {
                    return do_capability_viewportops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::PresentOps>)
                {
                    return do_capability_presentops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::InstancingOps>)
                {
                    return do_capability_instancingops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::OcclusionCullingOps>)
                {
                    return do_capability_occlusioncullingops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::QueueOps>)
                {
                    return do_capability_queueops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::CommandEncoderOps>)
                {
                    return do_capability_commandencoderops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::ShaderOps>)
                {
                    return do_capability_shaderops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::RenderPipelineOps>)
                {
                    return do_capability_renderpipelineops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::ComputePipelineOps>)
                {
                    return do_capability_computepipelineops();
                }
                else if constexpr (std::is_same_v<Capability, capabilities::BindGroupOps>)
                {
                    return do_capability_bindgroupops();
                }
                else
                {
                    std::unreachable();
                }
            }
        };

        /*!
         * @brief Model implementation wrapping concrete device type T.
         */
        template <typename T>
        struct Model : Concept
        {
            // Owning constructor
            explicit Model(T device_impl)
                : _device_storage(std::move(device_impl)), _device_ptr(&*_device_storage), _owns(true)
            {
                build_capability_map();
            }

            // Non-owning constructor
            explicit Model(T* device_ptr)
                : _device_storage(std::nullopt) // No storage in non-owning case
                  ,
                  _device_ptr(device_ptr), _owns(false)
            {
                build_capability_map();
            }

            bool supports(std::type_index ti) const override
            {
                return _capabilities.contains(ti);
            }

            std::optional<std::type_index> capability_type(std::type_index ti) const override
            {
                auto it = _capabilities.find(ti);
                return it != _capabilities.end() ? std::optional(ti) : std::nullopt;
            }

        private:
            std::optional<T>                              _device_storage; // Used if _owns == true
            T*                                            _device_ptr;     // Points to storage or external
            bool                                          _owns;           // Track ownership
            std::unordered_map<std::type_index, std::any> _capabilities;

            // Get pointer to device (handles both owning and non-owning cases)
            T* get_device() const
            {
                return _device_ptr;
            }

            void build_capability_map()
            {
                // Check for BufferOps capability
                if constexpr (requires(T& d, std::span<const std::byte> data, size_t sz, const Buffer& buf) {
                                  { d.create_vertex_buffer(data) } -> std::same_as<std::expected<Buffer, std::error_code>>;
                                  { d.create_index_buffer(data) } -> std::same_as<std::expected<Buffer, std::error_code>>;
                                  { d.create_uniform_buffer(sz) } -> std::same_as<std::expected<Buffer, std::error_code>>;
                                  { d.update_uniform_buffer(buf, data) } -> std::same_as<std::expected<void, std::error_code>>;
                                  { d.set_uniform_buffer(buf) } -> std::same_as<void>;
                              })
                {
                    _capabilities[std::type_index(typeid(capabilities::BufferOps))] = true;
                }

                // Check for DrawOps capability
                if constexpr (requires(T& d, const Buffer& vb, const Buffer& ib, uint32_t count) {
                                  { d.draw_indexed(vb, ib, count) } -> std::same_as<std::expected<void, std::error_code>>;
                                  { d.clear() } -> std::same_as<void>;
                              })
                {
                    _capabilities[std::type_index(typeid(capabilities::DrawOps))] = true;
                }

                // Check for ViewportOps capability
                if constexpr (requires(T& d, uint32_t w, uint32_t h, AspectRatio ar, float custom) {
                                  { d.resize(w, h) } -> std::same_as<std::expected<void, std::error_code>>;
                                  { d.set_aspect_ratio(ar, custom) } -> std::same_as<void>;
                                  { d.aspect_ratio() } -> std::same_as<AspectRatio>;
                                  { d.viewport() } -> std::same_as<const Viewport&>;
                              })
                {
                    _capabilities[std::type_index(typeid(capabilities::ViewportOps))] = true;
                }

                // Check for PresentOps capability
                if constexpr (requires(T& d) {
                                  { d.present() } -> std::same_as<void>;
                              })
                {
                    _capabilities[std::type_index(typeid(capabilities::PresentOps))] = true;
                }

                // Check for QueueOps capability
                if constexpr (requires(T& d) {
                                  { d.queue() } -> std::same_as<Queue>;
                              })
                {
                    _capabilities[std::type_index(typeid(capabilities::QueueOps))] = true;
                }

                // Check for CommandEncoderOps capability
                if constexpr (requires(T& d, std::string_view label) {
                                  { d.create_command_encoder(label) } -> std::same_as<CommandEncoder>;
                              })
                {
                    _capabilities[std::type_index(typeid(capabilities::CommandEncoderOps))] = true;
                }

                // ShaderOps capability
                if constexpr (requires(T& d, const ShaderModuleDescriptor& desc) {
                                  { d.create_shader_module(desc) } -> std::same_as<std::expected<ShaderModule, std::error_code>>;
                              })
                {
                    _capabilities[std::type_index(typeid(capabilities::ShaderOps))] = true;
                }

                // Check for RenderPipelineOps capability
                if constexpr (requires(T& d, const RenderPipelineDescriptor& desc) {
                                  { d.create_render_pipeline(desc) } -> std::same_as<std::expected<RenderPipeline, std::error_code>>;
                              })
                {
                    _capabilities[std::type_index(typeid(capabilities::RenderPipelineOps))] = true;
                }

                // Check for ComputePipelineOps capability
                if constexpr (requires(T& d, const ComputePipelineDescriptor& desc) {
                                  { d.create_compute_pipeline(desc) } -> std::same_as<std::expected<ComputePipeline, std::error_code>>;
                              })
                {
                    _capabilities[std::type_index(typeid(capabilities::ComputePipelineOps))] = true;
                }

                // Check for BindGroupOps capability
                if constexpr (requires(T& d, const BindGroupLayoutDescriptor& layout_desc, const BindGroupDescriptor& bind_desc) {
                                  { d.create_bind_group_layout(layout_desc) } -> std::same_as<std::expected<BindGroupLayout, std::error_code>>;
                                  { d.create_bind_group(bind_desc) } -> std::same_as<std::expected<BindGroup, std::error_code>>;
                              })
                {
                    _capabilities[std::type_index(typeid(capabilities::BindGroupOps))] = true;
                }
            }

            std::optional<capabilities::BufferOps> do_capability_bufferops() const override
            {
                if constexpr (requires(T& d, std::span<const std::byte> data, size_t sz, const Buffer& buf) {
                                  { d.create_vertex_buffer(data) } -> std::same_as<std::expected<Buffer, std::error_code>>;
                              })
                {
                    capabilities::BufferOps ops;
                    ops.create_vertex_buffer = [this](std::span<const std::byte> data) mutable
                    {
                        return this->get_device()->create_vertex_buffer(data);
                    };
                    ops.create_index_buffer = [this](std::span<const std::byte> data) mutable
                    {
                        return this->get_device()->create_index_buffer(data);
                    };
                    ops.create_uniform_buffer = [this](size_t sz) mutable
                    {
                        return this->get_device()->create_uniform_buffer(sz);
                    };
                    ops.update_uniform_buffer = [this](const Buffer& buf, std::span<const std::byte> data) mutable
                    {
                        return this->get_device()->update_uniform_buffer(buf, data);
                    };
                    ops.set_uniform_buffer = [this](const Buffer& buf) mutable
                    {
                        this->get_device()->set_uniform_buffer(buf);
                    };
                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::DrawOps> do_capability_drawops() const override
            {
                if constexpr (requires(T& d, const Buffer& vb, const Buffer& ib, uint32_t count) {
                                  { d.draw_indexed(vb, ib, count) } -> std::same_as<std::expected<void, std::error_code>>;
                              })
                {
                    capabilities::DrawOps ops;
                    ops.draw_indexed = [this](const Buffer& vb, const Buffer& ib, uint32_t count) mutable
                    {
                        return this->get_device()->draw_indexed(vb, ib, count);
                    };
                    ops.clear = [this]() mutable
                    {
                        this->get_device()->clear();
                    };
                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::ViewportOps> do_capability_viewportops() const override
            {
                if constexpr (requires(T& d, uint32_t w, uint32_t h, AspectRatio ar, float custom) {
                                  { d.resize(w, h) } -> std::same_as<std::expected<void, std::error_code>>;
                              })
                {
                    capabilities::ViewportOps ops;
                    ops.resize = [this](uint32_t w, uint32_t h) mutable
                    {
                        return this->get_device()->resize(w, h);
                    };
                    ops.set_aspect_ratio = [this](AspectRatio ar, float custom) mutable
                    {
                        this->get_device()->set_aspect_ratio(ar, custom);
                    };
                    ops.aspect_ratio = [this]() mutable
                    {
                        return this->get_device()->aspect_ratio();
                    };
                    ops.viewport = [this]() mutable -> const Viewport&
                    {
                        return this->get_device()->viewport();
                    };

                    // Optional surface view access (GPU devices only)
                    if constexpr (requires(T& d) {
                                      { d.get_surface_view() } -> std::convertible_to<void*>;
                                  })
                    {
                        ops.get_surface_view = [this]() mutable -> void*
                        {
                            return this->get_device()->get_surface_view();
                        };
                    }

                    // Optional depth view access (GPU devices only)
                    if constexpr (requires(T& d) {
                                      { d.get_depth_view() } -> std::convertible_to<void*>;
                                  })
                    {
                        ops.get_depth_view = [this]() mutable -> void*
                        {
                            return this->get_device()->get_depth_view();
                        };
                    }

                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::PresentOps> do_capability_presentops() const override
            {
                if constexpr (requires(T& d) {
                                  { d.present() } -> std::same_as<void>;
                              })
                {
                    capabilities::PresentOps ops;
                    ops.present = [this]() mutable
                    {
                        this->get_device()->present();
                    };
                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::InstancingOps> do_capability_instancingops() const override
            {
                if constexpr (requires(T& device, std::span<const std::byte> data, const Buffer& buffer, uint32_t count) {
                                  { device.create_instance_buffer(data) } -> std::same_as<std::expected<Buffer, std::error_code>>;
                                  { device.update_instance_buffer(buffer, data) } -> std::same_as<std::expected<void, std::error_code>>;
                                  { device.draw_indexed_instanced(buffer, buffer, buffer, count, count) } -> std::same_as<std::expected<void, std::error_code>>;
                              })
                {
                    capabilities::InstancingOps ops;
                    ops.create_instance_buffer = [this](std::span<const std::byte> data) mutable
                    {
                        return this->get_device()->create_instance_buffer(data);
                    };
                    ops.update_instance_buffer = [this](const Buffer& buffer, std::span<const std::byte> data) mutable
                    {
                        return this->get_device()->update_instance_buffer(buffer, data);
                    };
                    ops.draw_indexed_instanced = [this](const Buffer& vertex_buffer,
                                                        const Buffer& index_buffer,
                                                        const Buffer& instance_buffer,
                                                        uint32_t      index_count,
                                                        uint32_t      instance_count) mutable
                    {
                        return this->get_device()->draw_indexed_instanced(vertex_buffer, index_buffer, instance_buffer, index_count, instance_count);
                    };
                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::OcclusionCullingOps> do_capability_occlusioncullingops() const override
            {
                if constexpr (requires(T& device, uint32_t w, uint32_t h) {
                                  { device.create_hi_z_buffer(w, h) } -> std::same_as<std::expected<std::unique_ptr<occlusion::HiZBuffer>, std::error_code>>;
                              })
                {
                    capabilities::OcclusionCullingOps ops;
                    ops.create_hi_z_buffer = [this](uint32_t width, uint32_t height) mutable
                    {
                        return this->get_device()->create_hi_z_buffer(width, height);
                    };

                    // Add get_depth_texture if device supports it
                    if constexpr (requires(T& device) {
                                      { device.wgpu_depth_texture() } -> std::convertible_to<void*>;
                                  })
                    {
                        ops.get_depth_texture = [this]() mutable -> void*
                        {
                            return this->get_device()->wgpu_depth_texture();
                        };
                    }

                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::QueueOps> do_capability_queueops() const override
            {
                if constexpr (requires(T& device) {
                                  { device.queue() } -> std::same_as<Queue>;
                              })
                {
                    capabilities::QueueOps ops;
                    ops.queue = [this]() mutable
                    {
                        return this->get_device()->queue();
                    };
                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::CommandEncoderOps> do_capability_commandencoderops() const override
            {
                if constexpr (requires(T& device, std::string_view label) {
                                  { device.create_command_encoder(label) } -> std::same_as<CommandEncoder>;
                              })
                {
                    capabilities::CommandEncoderOps ops;
                    ops.create_command_encoder = [this](std::string_view label) mutable
                    {
                        return this->get_device()->create_command_encoder(label);
                    };
                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::ShaderOps> do_capability_shaderops() const override
            {
                if constexpr (requires(T& device, const ShaderModuleDescriptor& desc) {
                                  { device.create_shader_module(desc) } -> std::same_as<std::expected<ShaderModule, std::error_code>>;
                              })
                {
                    capabilities::ShaderOps ops;
                    ops.create_shader_module = [this](const ShaderModuleDescriptor& desc) mutable
                    {
                        return this->get_device()->create_shader_module(desc);
                    };
                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::RenderPipelineOps> do_capability_renderpipelineops() const override
            {
                if constexpr (requires(T& device, const RenderPipelineDescriptor& desc) {
                                  { device.create_render_pipeline(desc) } -> std::same_as<std::expected<RenderPipeline, std::error_code>>;
                              })
                {
                    capabilities::RenderPipelineOps ops;
                    ops.create_render_pipeline = [this](const RenderPipelineDescriptor& desc) mutable
                    {
                        return this->get_device()->create_render_pipeline(desc);
                    };
                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::ComputePipelineOps> do_capability_computepipelineops() const override
            {
                if constexpr (requires(T& device, const ComputePipelineDescriptor& desc) {
                                  { device.create_compute_pipeline(desc) } -> std::same_as<std::expected<ComputePipeline, std::error_code>>;
                              })
                {
                    capabilities::ComputePipelineOps ops;
                    ops.create_compute_pipeline = [this](const ComputePipelineDescriptor& desc) mutable
                    {
                        return this->get_device()->create_compute_pipeline(desc);
                    };
                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }

            std::optional<capabilities::BindGroupOps> do_capability_bindgroupops() const override
            {
                if constexpr (requires(T& device, const BindGroupLayoutDescriptor& layout_desc, const BindGroupDescriptor& bind_desc) {
                                  { device.create_bind_group_layout(layout_desc) } -> std::same_as<std::expected<BindGroupLayout, std::error_code>>;
                                  { device.create_bind_group(bind_desc) } -> std::same_as<std::expected<BindGroup, std::error_code>>;
                              })
                {
                    capabilities::BindGroupOps ops;
                    ops.create_bind_group_layout = [this](const BindGroupLayoutDescriptor& desc) mutable
                    {
                        return this->get_device()->create_bind_group_layout(desc);
                    };
                    ops.create_bind_group = [this](const BindGroupDescriptor& desc) mutable
                    {
                        return this->get_device()->create_bind_group(desc);
                    };
                    return ops;
                }
                else
                {
                    std::unreachable();
                }
            }
        };

        std::unique_ptr<Concept> _impl;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_DEVICE_H
