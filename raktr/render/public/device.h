/*!
 * @file device.h
 * @brief Non-owning type-erased Device view.
 *
 * This implementation uses a non-owning type erasure pattern (View) to provide
 * polymorphic access to device capabilities without inheritance or heap allocation.
 */

#ifndef RAKTR_RENDER_DEVICE_H
#define RAKTR_RENDER_DEVICE_H

#include "aspect_ratio.h"
#include "bind_group.h"
#include "buffer.h"
#include "command_encoder.h"
#include "compute_pipeline.h"
#include "device_capabilities.h"
#include "queue.h"
#include "render_pipeline.h"
#include "shader_module.h"
#include <any>
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>


namespace raktr::render
{
    /*!
     * @brief Non-owning type-erased view of a GPU device.
     *
     * This class does not own the underlying device. It provides a polymorphic
     * interface to any compatible device implementation (e.g. WgpuDevice, SoftDevice).
     * It is lightweight and copyable.
     */
    class DeviceView
    {
    public:
        // Default constructor creates an empty/invalid view
        DeviceView() = default;

        // Constructor from reference
        template <typename T>
            requires(!std::is_same_v<std::remove_cvref_t<T>, DeviceView>)
        DeviceView(T& device)
            : _object(const_cast<void*>(static_cast<const void*>(&device))), _vtable(&vtable_for<std::remove_cvref_t<T>>)
        {
        }

        // Constructor from pointer (allows null)
        template <typename T>
        DeviceView(T* device)
        {
            if (device)
            {
                _object = const_cast<void*>(static_cast<const void*>(device));
                _vtable = &vtable_for<std::remove_cvref_t<T>>;
            }
        }

        // Constructor from nullptr
        DeviceView(std::nullptr_t) : DeviceView()
        {
        }

        // Copyable
        DeviceView(const DeviceView&)            = default;
        DeviceView& operator=(const DeviceView&) = default;

        // Validity check
        explicit operator bool() const
        {
            return _object != nullptr;
        }

        // Comparison
        bool operator==(const DeviceView& other) const
        {
            return _object == other._object;
        }
        bool operator==(std::nullptr_t) const
        {
            return _object == nullptr;
        }

        // Pointer semantics
        const DeviceView* operator->() const
        {
            return this;
        }
        DeviceView* operator->()
        {
            return this;
        }

        /*!
         * @brief Check if device supports a specific capability.
         */
        template <typename Capability>
        [[nodiscard]] bool supports() const
        {
            if (!_object || !_vtable)
                return false;
            return _vtable->supports(_object, std::type_index(typeid(Capability)));
        }

        /*!
         * @brief Get a capability interface from the device.
         */
        template <typename Capability>
        [[nodiscard]] Capability capability() const
        {
            if (!_object || !_vtable)
                throw std::bad_optional_access();

            // Dispatch to specific getter based on type
            if constexpr (std::is_same_v<Capability, capabilities::BufferOps>)
                return _vtable->get_buffer_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::DrawOps>)
                return _vtable->get_draw_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::ViewportOps>)
                return _vtable->get_viewport_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::PresentOps>)
                return _vtable->get_present_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::InstancingOps>)
                return _vtable->get_instancing_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::OcclusionCullingOps>)
                return _vtable->get_occlusion_culling_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::QueueOps>)
                return _vtable->get_queue_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::CommandEncoderOps>)
                return _vtable->get_command_encoder_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::ShaderOps>)
                return _vtable->get_shader_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::RenderPipelineOps>)
                return _vtable->get_render_pipeline_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::ComputePipelineOps>)
                return _vtable->get_compute_pipeline_ops(_object).value();
            else if constexpr (std::is_same_v<Capability, capabilities::BindGroupOps>)
                return _vtable->get_bind_group_ops(_object).value();
            else
                static_assert(std::is_void_v<Capability>, "Unknown capability type");
        }

        // Convenience methods forwarding to capabilities

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
            return capability<capabilities::InstancingOps>().draw_indexed_instanced(vertex_buffer, index_buffer, instance_buffer, index_count, instance_count);
        }

        [[nodiscard]] void* get_depth_texture() const
        {
            return capability<capabilities::OcclusionCullingOps>().get_depth_texture();
        }

        [[nodiscard]] std::expected<ShaderModule, std::error_code>
        create_shader_module(const ShaderModuleDescriptor& descriptor) const
        {
            return capability<capabilities::ShaderOps>().create_shader_module(descriptor);
        }

        [[nodiscard]] std::expected<RenderPipeline, std::error_code>
        create_render_pipeline(const RenderPipelineDescriptor& descriptor) const
        {
            return capability<capabilities::RenderPipelineOps>().create_render_pipeline(descriptor);
        }

        [[nodiscard]] std::expected<ComputePipeline, std::error_code>
        create_compute_pipeline(const ComputePipelineDescriptor& descriptor) const
        {
            return capability<capabilities::ComputePipelineOps>().create_compute_pipeline(descriptor);
        }

        [[nodiscard]] std::expected<BindGroupLayout, std::error_code>
        create_bind_group_layout(const BindGroupLayoutDescriptor& descriptor) const
        {
            return capability<capabilities::BindGroupOps>().create_bind_group_layout(descriptor);
        }

        [[nodiscard]] std::expected<BindGroup, std::error_code>
        create_bind_group(const BindGroupDescriptor& descriptor) const
        {
            return capability<capabilities::BindGroupOps>().create_bind_group(descriptor);
        }

    private:
        struct VTable
        {
            bool (*supports)(const void*, std::type_index);
            std::optional<capabilities::BufferOps> (*get_buffer_ops)(const void*);
            std::optional<capabilities::DrawOps> (*get_draw_ops)(const void*);
            std::optional<capabilities::ViewportOps> (*get_viewport_ops)(const void*);
            std::optional<capabilities::PresentOps> (*get_present_ops)(const void*);
            std::optional<capabilities::InstancingOps> (*get_instancing_ops)(const void*);
            std::optional<capabilities::OcclusionCullingOps> (*get_occlusion_culling_ops)(const void*);
            std::optional<capabilities::QueueOps> (*get_queue_ops)(const void*);
            std::optional<capabilities::CommandEncoderOps> (*get_command_encoder_ops)(const void*);
            std::optional<capabilities::ShaderOps> (*get_shader_ops)(const void*);
            std::optional<capabilities::RenderPipelineOps> (*get_render_pipeline_ops)(const void*);
            std::optional<capabilities::ComputePipelineOps> (*get_compute_pipeline_ops)(const void*);
            std::optional<capabilities::BindGroupOps> (*get_bind_group_ops)(const void*);
        };

        void*         _object = nullptr;
        const VTable* _vtable = nullptr;

        template <typename T>
#pragma warning(push)
#pragma warning(disable : 4268)
        static constexpr VTable vtable_for = {
            .supports = [](const void* /*ptr*/, std::type_index ti) -> bool
            {
                // Check compile-time constraints and map to capabilities
                if (ti == std::type_index(typeid(capabilities::BufferOps)))
                    return requires(T& d, std::span<const std::byte> data, size_t sz, const Buffer& buf) {
                        { d.create_vertex_buffer(data) } -> std::same_as<std::expected<Buffer, std::error_code>>;
                        { d.create_index_buffer(data) } -> std::same_as<std::expected<Buffer, std::error_code>>;
                        { d.create_uniform_buffer(sz) } -> std::same_as<std::expected<Buffer, std::error_code>>;
                        { d.update_uniform_buffer(buf, data) } -> std::same_as<std::expected<void, std::error_code>>;
                        { d.set_uniform_buffer(buf) } -> std::same_as<void>;
                    };
                if (ti == std::type_index(typeid(capabilities::DrawOps)))
                    return requires(T& d, const Buffer& vb, const Buffer& ib, uint32_t count) {
                        { d.draw_indexed(vb, ib, count) } -> std::same_as<std::expected<void, std::error_code>>;
                        { d.clear() } -> std::same_as<void>;
                    };
                if (ti == std::type_index(typeid(capabilities::ViewportOps)))
                    return requires(T& d, uint32_t w, uint32_t h, AspectRatio ar, float custom) {
                        { d.resize(w, h) } -> std::same_as<std::expected<void, std::error_code>>;
                        { d.set_aspect_ratio(ar, custom) } -> std::same_as<void>;
                        { d.aspect_ratio() } -> std::same_as<AspectRatio>;
                        { d.viewport() } -> std::same_as<const Viewport&>;
                    };
                if (ti == std::type_index(typeid(capabilities::PresentOps)))
                    return requires(T& d) { { d.present() } -> std::same_as<void>; };
                if (ti == std::type_index(typeid(capabilities::QueueOps)))
                    return requires(T& d) { { d.queue() } -> std::same_as<Queue>; };
                if (ti == std::type_index(typeid(capabilities::CommandEncoderOps)))
                    return requires(T& d, std::string_view l) { { d.create_command_encoder(l) } -> std::same_as<CommandEncoder>; };
                if (ti == std::type_index(typeid(capabilities::ShaderOps)))
                    return requires(T& d, const ShaderModuleDescriptor& desc) { { d.create_shader_module(desc) } -> std::same_as<std::expected<ShaderModule, std::error_code>>; };
                if (ti == std::type_index(typeid(capabilities::RenderPipelineOps)))
                    return requires(T& d, const RenderPipelineDescriptor& desc) { { d.create_render_pipeline(desc) } -> std::same_as<std::expected<RenderPipeline, std::error_code>>; };
                if (ti == std::type_index(typeid(capabilities::ComputePipelineOps)))
                    return requires(T& d, const ComputePipelineDescriptor& desc) { { d.create_compute_pipeline(desc) } -> std::same_as<std::expected<ComputePipeline, std::error_code>>; };
                if (ti == std::type_index(typeid(capabilities::BindGroupOps)))
                    return requires(T& d, const BindGroupLayoutDescriptor& l, const BindGroupDescriptor& b) {
                        { d.create_bind_group_layout(l) } -> std::same_as<std::expected<BindGroupLayout, std::error_code>>;
                        { d.create_bind_group(b) } -> std::same_as<std::expected<BindGroup, std::error_code>>;
                    };
                if (ti == std::type_index(typeid(capabilities::InstancingOps)))
                    return requires(T& d, std::span<const std::byte> s, const Buffer& b, uint32_t c) {
                        { d.create_instance_buffer(s) } -> std::same_as<std::expected<Buffer, std::error_code>>;
                        { d.update_instance_buffer(b, s) } -> std::same_as<std::expected<void, std::error_code>>;
                        { d.draw_indexed_instanced(b, b, b, c, c) } -> std::same_as<std::expected<void, std::error_code>>;
                    };
                if (ti == std::type_index(typeid(capabilities::OcclusionCullingOps)))
                    return requires(T& d, uint32_t w, uint32_t h) {
                        { d.create_hi_z_buffer(w, h) } -> std::same_as<std::expected<std::unique_ptr<occlusion::HiZBuffer>, std::error_code>>;
                    };
                return false;
            },
            .get_buffer_ops = [](const void* ptr) -> std::optional<capabilities::BufferOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev, std::span<const std::byte> dat, size_t sz, const Buffer& buf) {
                                  { dev.create_vertex_buffer(dat) };
                              }) // simplified check
                {
                    capabilities::BufferOps ops;
                    ops.create_vertex_buffer = [d](std::span<const std::byte> da)
                    {
                        return d->create_vertex_buffer(da);
                    };
                    ops.create_index_buffer = [d](std::span<const std::byte> da)
                    {
                        return d->create_index_buffer(da);
                    };
                    ops.create_uniform_buffer = [d](size_t sz)
                    {
                        return d->create_uniform_buffer(sz);
                    };
                    ops.update_uniform_buffer = [d](const Buffer& b, std::span<const std::byte> da)
                    {
                        return d->update_uniform_buffer(b, da);
                    };
                    ops.set_uniform_buffer = [d](const Buffer& b)
                    {
                        d->set_uniform_buffer(b);
                    };
                    return ops;
                }
                return std::nullopt;
            },
            .get_draw_ops = [](const void* ptr) -> std::optional<capabilities::DrawOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev) { { dev.clear() }; })
                {
                    capabilities::DrawOps ops;
                    ops.draw_indexed = [d](const Buffer& vb, const Buffer& ib, uint32_t c)
                    {
                        return d->draw_indexed(vb, ib, c);
                    };
                    ops.clear = [d]()
                    {
                        d->clear();
                    };
                    return ops;
                }
                return std::nullopt;
            },
            .get_viewport_ops = [](const void* ptr) -> std::optional<capabilities::ViewportOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev) { { dev.aspect_ratio() }; })
                {
                    capabilities::ViewportOps ops;
                    ops.resize = [d](uint32_t w, uint32_t h)
                    {
                        return d->resize(w, h);
                    };
                    ops.set_aspect_ratio = [d](AspectRatio ar, float c)
                    {
                        d->set_aspect_ratio(ar, c);
                    };
                    ops.aspect_ratio = [d]()
                    {
                        return d->aspect_ratio();
                    };
                    ops.viewport = [d]() -> const Viewport&
                    {
                        return d->viewport();
                    };

                    if constexpr (requires(T& dev) { { dev.get_surface_view() } -> std::convertible_to<void*>; })
                        ops.get_surface_view = [d]()
                        {
                            return d->get_surface_view();
                        };
                    if constexpr (requires(T& dev) { { dev.get_depth_view() } -> std::convertible_to<void*>; })
                        ops.get_depth_view = [d]()
                        {
                            return d->get_depth_view();
                        };
                    return ops;
                }
                return std::nullopt;
            },
            .get_present_ops = [](const void* ptr) -> std::optional<capabilities::PresentOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev) { { dev.present() }; })
                {
                    return capabilities::PresentOps{ .present = [d]()
                                                     {
                                                         d->present();
                                                     } };
                }
                return std::nullopt;
            },
            .get_instancing_ops = [](const void* ptr) -> std::optional<capabilities::InstancingOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev, std::span<const std::byte> s, const Buffer& b, uint32_t c) {
                                  { dev.create_instance_buffer(s) };
                              })
                {
                    capabilities::InstancingOps ops;
                    ops.create_instance_buffer = [d](std::span<const std::byte> s)
                    {
                        return d->create_instance_buffer(s);
                    };
                    ops.update_instance_buffer = [d](const Buffer& b, std::span<const std::byte> s)
                    {
                        return d->update_instance_buffer(b, s);
                    };
                    ops.draw_indexed_instanced = [d](const Buffer& vb, const Buffer& ib, const Buffer& kb, uint32_t ic, uint32_t nc)
                    {
                        return d->draw_indexed_instanced(vb, ib, kb, ic, nc);
                    };
                    return ops;
                }
                return std::nullopt;
            },
            .get_occlusion_culling_ops = [](const void* ptr) -> std::optional<capabilities::OcclusionCullingOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev, uint32_t w, uint32_t h) { { dev.create_hi_z_buffer(w, h) }; })
                {
                    capabilities::OcclusionCullingOps ops;
                    ops.create_hi_z_buffer = [d](uint32_t w, uint32_t h)
                    {
                        return d->create_hi_z_buffer(w, h);
                    };
                    if constexpr (requires(T& dev) { { dev.wgpu_depth_texture() }; })
                        ops.get_depth_texture = [d]()
                        {
                            return d->wgpu_depth_texture();
                        };
                    return ops;
                }
                return std::nullopt;
            },
            .get_queue_ops = [](const void* ptr) -> std::optional<capabilities::QueueOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev) { { dev.queue() }; })
                {
                    return capabilities::QueueOps{ .queue = [d]()
                                                   {
                                                       return d->queue();
                                                   } };
                }
                return std::nullopt;
            },
            .get_command_encoder_ops = [](const void* ptr) -> std::optional<capabilities::CommandEncoderOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev, std::string_view s) { { dev.create_command_encoder(s) }; })
                {
                    return capabilities::CommandEncoderOps{ .create_command_encoder = [d](std::string_view s)
                                                            {
                                                                return d->create_command_encoder(s);
                                                            } };
                }
                return std::nullopt;
            },
            .get_shader_ops = [](const void* ptr) -> std::optional<capabilities::ShaderOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev, const ShaderModuleDescriptor& s) { { dev.create_shader_module(s) }; })
                {
                    return capabilities::ShaderOps{ .create_shader_module = [d](const ShaderModuleDescriptor& s)
                                                    {
                                                        return d->create_shader_module(s);
                                                    } };
                }
                return std::nullopt;
            },
            .get_render_pipeline_ops = [](const void* ptr) -> std::optional<capabilities::RenderPipelineOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev, const RenderPipelineDescriptor& s) { { dev.create_render_pipeline(s) }; })
                {
                    return capabilities::RenderPipelineOps{ .create_render_pipeline = [d](const RenderPipelineDescriptor& s)
                                                            {
                                                                return d->create_render_pipeline(s);
                                                            } };
                }
                return std::nullopt;
            },
            .get_compute_pipeline_ops = [](const void* ptr) -> std::optional<capabilities::ComputePipelineOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev, const ComputePipelineDescriptor& s) { { dev.create_compute_pipeline(s) }; })
                {
                    return capabilities::ComputePipelineOps{ .create_compute_pipeline = [d](const ComputePipelineDescriptor& s)
                                                             {
                                                                 return d->create_compute_pipeline(s);
                                                             } };
                }
                return std::nullopt;
            },
            .get_bind_group_ops = [](const void* ptr) -> std::optional<capabilities::BindGroupOps>
            {
                [[maybe_unused]] T* d = const_cast<T*>(static_cast<const T*>(ptr));
                if constexpr (requires(T& dev, const BindGroupLayoutDescriptor& l) { { dev.create_bind_group_layout(l) }; })
                {
                    capabilities::BindGroupOps ops;
                    ops.create_bind_group_layout = [d](const BindGroupLayoutDescriptor& l)
                    {
                        return d->create_bind_group_layout(l);
                    };
                    ops.create_bind_group = [d](const BindGroupDescriptor& b)
                    {
                        return d->create_bind_group(b);
                    };
                    return ops;
                }
                return std::nullopt;
            },
        };
#pragma warning(pop)
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_DEVICE_H
