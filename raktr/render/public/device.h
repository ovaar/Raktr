/*!
 * @file device.h
 * @brief Type-erased Device value type.
 *
 * This implementation uses the External Polymorphism Design Pattern with a Bridge (Pimpl)
 * to provide a value-semantics, type-erased Device class.
 */

#ifndef RAKTR_RENDER_DEVICE_H
#define RAKTR_RENDER_DEVICE_H

#include "aspect_ratio.h"
#include "bind_group.h"
#include "buffer.h"
#include "command_encoder.h"
#include "compute_pipeline.h"
#include "device_capabilities.h"
#include "instance_data.h"
#include "queue.h"
#include "render_pass.h"
#include "render_pipeline.h"
#include "shader_module.h"


#include <concepts>
#include <cstddef>
#include <expected>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>


namespace raktr::render
{
    // Forward declarations of free functions for External Polymorphism
    // We provide default implementations that forward to member functions if they exist.

    // Buffer Ops
    template <typename T>
    auto create_vertex_buffer(const T& t, std::span<const std::byte> data)
        -> std::expected<Buffer, std::error_code>
    {
        if constexpr (requires { t.create_vertex_buffer(data); })
            return t.create_vertex_buffer(data);
        else if constexpr (requires { t->create_vertex_buffer(data); })
            return t->create_vertex_buffer(data);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    template <typename T>
    auto create_index_buffer(const T& t, std::span<const std::byte> data)
        -> std::expected<Buffer, std::error_code>
    {
        if constexpr (requires { t.create_index_buffer(data); })
            return t.create_index_buffer(data);
        else if constexpr (requires { t->create_index_buffer(data); })
            return t->create_index_buffer(data);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    template <typename T>
    auto create_uniform_buffer(const T& t, size_t size)
        -> std::expected<Buffer, std::error_code>
    {
        if constexpr (requires { t.create_uniform_buffer(size); })
            return t.create_uniform_buffer(size);
        else if constexpr (requires { t->create_uniform_buffer(size); })
            return t->create_uniform_buffer(size);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    template <typename T>
    auto update_uniform_buffer(const T& t, const Buffer& buffer, std::span<const std::byte> data)
        -> std::expected<void, std::error_code>
    {
        if constexpr (requires { t.update_uniform_buffer(buffer, data); })
            return t.update_uniform_buffer(buffer, data);
        else if constexpr (requires { t->update_uniform_buffer(buffer, data); })
            return t->update_uniform_buffer(buffer, data);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    template <typename T>
    void set_uniform_buffer(const T& t, const Buffer& buffer)
    {
        if constexpr (requires { t.set_uniform_buffer(buffer); })
            t.set_uniform_buffer(buffer);
        else if constexpr (requires { t->set_uniform_buffer(buffer); })
            t->set_uniform_buffer(buffer);
    }

    // Draw Ops
    template <typename T>
    auto draw_indexed(const T& t, const Buffer& vb, const Buffer& ib, uint32_t count)
        -> std::expected<void, std::error_code>
    {
        if constexpr (requires { t.draw_indexed(vb, ib, count); })
            return t.draw_indexed(vb, ib, count);
        else if constexpr (requires { t->draw_indexed(vb, ib, count); })
            return t->draw_indexed(vb, ib, count);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    template <typename T>
    void clear(const T& t)
    {
        if constexpr (requires { t.clear(); })
            t.clear();
        else if constexpr (requires { t->clear(); })
            t->clear();
    }

    // Viewport Ops
    template <typename T>
    auto resize(const T& t, uint32_t w, uint32_t h) -> std::expected<void, std::error_code>
    {
        if constexpr (requires { t.resize(w, h); })
            return t.resize(w, h);
        else if constexpr (requires { t->resize(w, h); })
            return t->resize(w, h);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    template <typename T>
    void set_aspect_ratio(const T& t, AspectRatio ratio, float custom)
    {
        if constexpr (requires { t.set_aspect_ratio(ratio, custom); })
            t.set_aspect_ratio(ratio, custom);
        else if constexpr (requires { t->set_aspect_ratio(ratio, custom); })
            t->set_aspect_ratio(ratio, custom);
    }

    template <typename T>
    auto aspect_ratio(const T& t) -> AspectRatio
    {
        if constexpr (requires { t.aspect_ratio(); })
            return t.aspect_ratio();
        else if constexpr (requires { t->aspect_ratio(); })
            return t->aspect_ratio();
        else
            return AspectRatio::Custom;
    }

    template <typename T>
    auto viewport(const T& t) -> const Viewport&
    {
        static const Viewport empty_vp{};
        if constexpr (requires { t.viewport(); })
            return t.viewport();
        else if constexpr (requires { t->viewport(); })
            return t->viewport();
        else
            return empty_vp;
    }

    template <typename T>
    void* get_surface_view(const T& t)
    {
        if constexpr (requires { t.get_surface_view(); })
            return t.get_surface_view();
        else if constexpr (requires { t->get_surface_view(); })
            return t->get_surface_view();
        else
            return nullptr;
    }

    template <typename T>
    void* get_depth_view(const T& t)
    {
        if constexpr (requires { t.get_depth_view(); })
            return t.get_depth_view();
        else if constexpr (requires { t->get_depth_view(); })
            return t->get_depth_view();
        else
            return nullptr;
    }

    // Present Ops
    template <typename T>
    void present(const T& t)
    {
        if constexpr (requires { t.present(); })
            t.present();
        else if constexpr (requires { t->present(); })
            t->present();
    }

    // Queue Ops
    template <typename T>
    auto queue(const T& t) -> std::expected<Queue, std::error_code>
    {
        if constexpr (requires { t.queue(); })
            return t.queue();
        else if constexpr (requires { t->queue(); })
            return t->queue();
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    // Command Encoder Ops
    template <typename T>
    auto create_command_encoder(const T& t, std::string_view label) -> std::expected<CommandEncoder, std::error_code>
    {
        if constexpr (requires { t.create_command_encoder(label); })
            return t.create_command_encoder(label);
        else if constexpr (requires { t->create_command_encoder(label); })
            return t->create_command_encoder(label);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    // Shader Ops
    template <typename T>
    auto create_shader_module(const T& t, const ShaderModuleDescriptor& desc)
        -> std::expected<ShaderModule, std::error_code>
    {
        if constexpr (requires { t.create_shader_module(desc); })
            return t.create_shader_module(desc);
        else if constexpr (requires { t->create_shader_module(desc); })
            return t->create_shader_module(desc);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    // Render Pipeline Ops
    template <typename T>
    auto create_render_pipeline(const T& t, const RenderPipelineDescriptor& desc)
        -> std::expected<RenderPipeline, std::error_code>
    {
        if constexpr (requires { t.create_render_pipeline(desc); })
            return t.create_render_pipeline(desc);
        else if constexpr (requires { t->create_render_pipeline(desc); })
            return t->create_render_pipeline(desc);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    // Compute Pipeline Ops
    template <typename T>
    auto create_compute_pipeline(const T& t, const ComputePipelineDescriptor& desc)
        -> std::expected<ComputePipeline, std::error_code>
    {
        if constexpr (requires { t.create_compute_pipeline(desc); })
            return t.create_compute_pipeline(desc);
        else if constexpr (requires { t->create_compute_pipeline(desc); })
            return t->create_compute_pipeline(desc);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    // Bind Group Ops
    template <typename T>
    auto create_bind_group_layout(const T& t, const BindGroupLayoutDescriptor& desc)
        -> std::expected<BindGroupLayout, std::error_code>
    {
        if constexpr (requires { t.create_bind_group_layout(desc); })
            return t.create_bind_group_layout(desc);
        else if constexpr (requires { t->create_bind_group_layout(desc); })
            return t->create_bind_group_layout(desc);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    template <typename T>
    auto create_bind_group(const T& t, const BindGroupDescriptor& desc)
        -> std::expected<BindGroup, std::error_code>
    {
        if constexpr (requires { t.create_bind_group(desc); })
            return t.create_bind_group(desc);
        else if constexpr (requires { t->create_bind_group(desc); })
            return t->create_bind_group(desc);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    // Instancing Ops
    template <typename T>
    auto create_instance_buffer(const T& t, std::span<const std::byte> data)
        -> std::expected<Buffer, std::error_code>
    {
        if constexpr (requires { t.create_instance_buffer(data); })
            return t.create_instance_buffer(data);
        else if constexpr (requires { t->create_instance_buffer(data); })
            return t->create_instance_buffer(data);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    template <typename T>
    auto update_instance_buffer(const T& t, const Buffer& buffer, std::span<const std::byte> data)
        -> std::expected<void, std::error_code>
    {
        if constexpr (requires { t.update_instance_buffer(buffer, data); })
            return t.update_instance_buffer(buffer, data);
        else if constexpr (requires { t->update_instance_buffer(buffer, data); })
            return t->update_instance_buffer(buffer, data);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    template <typename T>
    auto draw_indexed_instanced(const T& t, const Buffer& vb, const Buffer& ib, const Buffer& instb, uint32_t ic, uint32_t instc)
        -> std::expected<void, std::error_code>
    {
        if constexpr (requires { t.draw_indexed_instanced(vb, ib, instb, ic, instc); })
            return t.draw_indexed_instanced(vb, ib, instb, ic, instc);
        else if constexpr (requires { t->draw_indexed_instanced(vb, ib, instb, ic, instc); })
            return t->draw_indexed_instanced(vb, ib, instb, ic, instc);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    // Occlusion Culling Ops
    template <typename T>
    auto create_hi_z_buffer(const T& t, uint32_t w, uint32_t h)
        -> std::expected<std::unique_ptr<occlusion::HiZBuffer>, std::error_code>
    {
        if constexpr (requires { t.create_hi_z_buffer(w, h); })
            return t.create_hi_z_buffer(w, h);
        else if constexpr (requires { t->create_hi_z_buffer(w, h); })
            return t->create_hi_z_buffer(w, h);
        else
            return std::unexpected(std::make_error_code(std::errc::function_not_supported));
    }

    template <typename T>
    auto create_hi_z_pyramid_pass(const T& t, occlusion::HiZBuffer* buffer, void* depth_texture)
        -> RenderPass
    {
        if constexpr (requires { t.create_hi_z_pyramid_pass(buffer, depth_texture); })
            return t.create_hi_z_pyramid_pass(buffer, depth_texture);
        else if constexpr (requires { t->create_hi_z_pyramid_pass(buffer, depth_texture); })
            return t->create_hi_z_pyramid_pass(buffer, depth_texture);
        else
            throw std::runtime_error("Backend does not support Hi-Z pyramid pass");
        // Actually RenderPass needs a valid pass object.
        // If backend doesn't support it, we can't return a valid RenderPass easily unless we have a NullPass.
        // For now assume support or return a dummy.
        // But RenderPass constructor is template. RenderPass(0) might fail or be weird.
        // Let's assume we return a valid pass or throw?
        // Existing code returns std::unexpected for others. RenderPass is not expected-wrapped here.
        // Let's wrap in expected? Not requested in plan but safer.
        // Plan said "RenderPass create...".
        // I'll assume support for now or simple "fail".
        // RenderPass constructor from int is likely invalid.
        // Let's try to match existing pattern. If return type is RenderPass, we can't use unexpected.
    }

    template <typename T>
    auto create_hi_z_occlusion_pass(const T& t, occlusion::HiZBuffer* buffer, const std::vector<occlusion::AABB>* aabbs, const glm::mat4& vp, std::vector<bool>* results)
        -> RenderPass
    {
        if constexpr (requires { t.create_hi_z_occlusion_pass(buffer, aabbs, vp, results); })
            return t.create_hi_z_occlusion_pass(buffer, aabbs, vp, results);
        else if constexpr (requires { t->create_hi_z_occlusion_pass(buffer, aabbs, vp, results); })
            return t->create_hi_z_occlusion_pass(buffer, aabbs, vp, results);
        else
            throw std::runtime_error("Backend does not support Hi-Z occlusion pass");
    }

    class RenderPass;
    struct InstanceData; // forward declare? public/instance_data.h is likely needed if used in sig.

    // ... inside Device ...
    template <typename T>
    auto get_depth_texture(const T& t) -> void*
    {
        if constexpr (requires { t.wgpu_depth_texture(); })
            return t.wgpu_depth_texture();
        else if constexpr (requires { t->wgpu_depth_texture(); })
            return t->wgpu_depth_texture();
        else if constexpr (requires { t.get_depth_texture(); })
            return t.get_depth_texture();
        else if constexpr (requires { t->get_depth_texture(); })
            return t->get_depth_texture();
        else
            return nullptr;
    }

    template <typename T>
    auto create_instanced_geometry_pass(const T&                   t,
                                        Buffer                     vb,
                                        Buffer                     ib,
                                        Buffer                     instb,
                                        std::vector<InstanceData>* cpu_data,
                                        std::vector<bool>*         visibility,
                                        uint32_t                   index_count)
        -> RenderPass
    {
        if constexpr (requires { t.create_instanced_geometry_pass(vb, ib, instb, cpu_data, visibility, index_count); })
            return t.create_instanced_geometry_pass(vb, ib, instb, cpu_data, visibility, index_count);
        else if constexpr (requires { t->create_instanced_geometry_pass(vb, ib, instb, cpu_data, visibility, index_count); })
            return t->create_instanced_geometry_pass(vb, ib, instb, cpu_data, visibility, index_count);
        else
            throw std::runtime_error("Backend does not support Instanced Geometry pass");
    }

    template <typename T>
    concept IsDevice = true;

    /*!
     * @brief Type-erased Device class.
     */
    class Device
    {
    public:
        // Default constructor creates invalid device
        Device() = default;

        // Constructor from value (owning) or pointer (view)
        template <IsDevice T>
            requires(!std::is_same_v<std::remove_cvref_t<T>, Device>)
        Device(T x) : _pimpl{ std::make_unique<DeviceModel<T>>(std::move(x)) }
        {
        }

        // Copy operations - Deep copy via clone
        Device(const Device& other) : _pimpl(other._pimpl ? other._pimpl->clone() : nullptr)
        {
        }
        Device& operator=(const Device& other)
        {
            _pimpl = other._pimpl ? other._pimpl->clone() : nullptr;
            return *this;
        }

        // Move operations
        Device(Device&&) noexcept            = default;
        Device& operator=(Device&&) noexcept = default;

        // Validity check
        explicit operator bool() const
        {
            return _pimpl != nullptr;
        }

        // Convenience pointer access
        const Device* operator->() const
        {
            return this;
        }
        Device* operator->()
        {
            return this;
        }

        // Comparison
        bool operator==(const Device& other) const
        {
            if (!_pimpl && !other._pimpl)
                return true;
            return false;
        }
        bool operator==(std::nullptr_t) const
        {
            return _pimpl == nullptr;
        }

        // --- Capability Queries (Backward Compatibility) ---
        template <typename Capability>
        [[nodiscard]] bool supports() const
        {
            if (!_pimpl)
                return false;
            return _pimpl->supports(std::type_index(typeid(Capability)));
        }

        template <typename Capability>
        [[nodiscard]] Capability capability() const;

        // --- Public Interface ---

        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_vertex_buffer(std::span<const std::byte> data) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_vertex_buffer(data);
        }

        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_index_buffer(std::span<const std::byte> data) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_index_buffer(data);
        }

        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_uniform_buffer(size_t size) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_uniform_buffer(size);
        }

        [[nodiscard]] std::expected<void, std::error_code>
        update_uniform_buffer(const Buffer& buffer, std::span<const std::byte> data) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->update_uniform_buffer(buffer, data);
        }

        void set_uniform_buffer(const Buffer& buffer) const
        {
            if (_pimpl)
                _pimpl->set_uniform_buffer(buffer);
        }

        [[nodiscard]] std::expected<void, std::error_code>
        draw_indexed(const Buffer& vb, const Buffer& ib, uint32_t count) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->draw_indexed(vb, ib, count);
        }

        void clear() const
        {
            if (_pimpl)
                _pimpl->clear();
        }

        [[nodiscard]] std::expected<void, std::error_code>
        resize(uint32_t w, uint32_t h) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->resize(w, h);
        }

        void set_aspect_ratio(AspectRatio ratio, float custom = 0.0f) const
        {
            if (_pimpl)
                _pimpl->set_aspect_ratio(ratio, custom);
        }

        [[nodiscard]] AspectRatio aspect_ratio() const
        {
            return _pimpl ? _pimpl->aspect_ratio() : AspectRatio::Custom;
        }

        [[nodiscard]] const Viewport& viewport() const
        {
            static const Viewport empty{};
            return _pimpl ? _pimpl->viewport() : empty;
        }

        [[nodiscard]] void* get_surface_view() const
        {
            return _pimpl ? _pimpl->get_surface_view() : nullptr;
        }

        [[nodiscard]] void* get_depth_view() const
        {
            return _pimpl ? _pimpl->get_depth_view() : nullptr;
        }

        void present() const
        {
            if (_pimpl)
                _pimpl->present();
        }

        [[nodiscard]] std::expected<Queue, std::error_code> queue() const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->queue();
        }

        [[nodiscard]] std::expected<CommandEncoder, std::error_code> create_command_encoder(std::string_view label = "") const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_command_encoder(label);
        }

        [[nodiscard]] std::expected<ShaderModule, std::error_code>
        create_shader_module(const ShaderModuleDescriptor& desc) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_shader_module(desc);
        }

        [[nodiscard]] std::expected<RenderPipeline, std::error_code>
        create_render_pipeline(const RenderPipelineDescriptor& desc) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_render_pipeline(desc);
        }

        [[nodiscard]] std::expected<ComputePipeline, std::error_code>
        create_compute_pipeline(const ComputePipelineDescriptor& desc) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_compute_pipeline(desc);
        }

        [[nodiscard]] std::expected<BindGroupLayout, std::error_code>
        create_bind_group_layout(const BindGroupLayoutDescriptor& desc) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_bind_group_layout(desc);
        }

        [[nodiscard]] std::expected<BindGroup, std::error_code>
        create_bind_group(const BindGroupDescriptor& desc) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_bind_group(desc);
        }

        [[nodiscard]] std::expected<Buffer, std::error_code>
        create_instance_buffer(std::span<const std::byte> data) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_instance_buffer(data);
        }

        [[nodiscard]] std::expected<void, std::error_code>
        update_instance_buffer(const Buffer& b, std::span<const std::byte> d) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->update_instance_buffer(b, d);
        }

        [[nodiscard]] std::expected<void, std::error_code>
        draw_indexed_instanced(const Buffer& vb, const Buffer& ib, const Buffer& kb, uint32_t ic, uint32_t nc) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->draw_indexed_instanced(vb, ib, kb, ic, nc);
        }

        [[nodiscard]] std::expected<std::unique_ptr<occlusion::HiZBuffer>, std::error_code>
        create_hi_z_buffer(uint32_t w, uint32_t h) const
        {
            if (!_pimpl)
                return std::unexpected(std::make_error_code(std::errc::invalid_argument));
            return _pimpl->create_hi_z_buffer(w, h);
        }

        [[nodiscard]] RenderPass create_hi_z_pyramid_pass(occlusion::HiZBuffer* buffer, void* depth_texture) const
        {
            if (!_pimpl)
                throw std::runtime_error("Device not initialized"); // Should match error handling strategy
            return _pimpl->create_hi_z_pyramid_pass(buffer, depth_texture);
        }

        [[nodiscard]] RenderPass create_hi_z_occlusion_pass(occlusion::HiZBuffer*               buffer,
                                                            const std::vector<occlusion::AABB>* aabbs,
                                                            const glm::mat4&                    vp,
                                                            std::vector<bool>*                  results) const
        {
            if (!_pimpl)
                throw std::runtime_error("Device not initialized");
            return _pimpl->create_hi_z_occlusion_pass(buffer, aabbs, vp, results);
        }

        [[nodiscard]] RenderPass create_instanced_geometry_pass(
            Buffer vb, Buffer ib, Buffer instb, std::vector<InstanceData>* cpu_data, std::vector<bool>* visibility, uint32_t index_count) const
        {
            if (!_pimpl)
                throw std::runtime_error("Device not initialized");
            return _pimpl->create_instanced_geometry_pass(vb, ib, instb, cpu_data, visibility, index_count);
        }

        [[nodiscard]] void* get_depth_texture() const
        {
            return _pimpl ? _pimpl->get_depth_texture() : nullptr;
        }

    private:
        class DeviceConcept
        {
        public:
            virtual ~DeviceConcept()                                                  = default;
            virtual std::unique_ptr<DeviceConcept> clone() const                      = 0;
            virtual bool                           supports(std::type_index ti) const = 0;

            virtual std::expected<Buffer, std::error_code> create_vertex_buffer(std::span<const std::byte>) const                 = 0;
            virtual std::expected<Buffer, std::error_code> create_index_buffer(std::span<const std::byte>) const                  = 0;
            virtual std::expected<Buffer, std::error_code> create_uniform_buffer(size_t) const                                    = 0;
            virtual std::expected<void, std::error_code>   update_uniform_buffer(const Buffer&, std::span<const std::byte>) const = 0;
            virtual void                                   set_uniform_buffer(const Buffer&) const                                = 0;

            virtual std::expected<void, std::error_code> draw_indexed(const Buffer&, const Buffer&, uint32_t) const = 0;
            virtual void                                 clear() const                                              = 0;

            virtual std::expected<void, std::error_code> resize(uint32_t, uint32_t) const           = 0;
            virtual void                                 set_aspect_ratio(AspectRatio, float) const = 0;
            virtual AspectRatio                          aspect_ratio() const                       = 0;
            virtual const Viewport&                      viewport() const                           = 0;
            virtual void*                                get_surface_view() const                   = 0;
            virtual void*                                get_depth_view() const                     = 0;

            virtual void                                           present() const                                = 0;
            virtual std::expected<Queue, std::error_code>          queue() const                                  = 0;
            virtual std::expected<CommandEncoder, std::error_code> create_command_encoder(std::string_view) const = 0;

            virtual std::expected<ShaderModule, std::error_code>    create_shader_module(const ShaderModuleDescriptor&) const       = 0;
            virtual std::expected<RenderPipeline, std::error_code>  create_render_pipeline(const RenderPipelineDescriptor&) const   = 0;
            virtual std::expected<ComputePipeline, std::error_code> create_compute_pipeline(const ComputePipelineDescriptor&) const = 0;

            virtual std::expected<BindGroupLayout, std::error_code> create_bind_group_layout(const BindGroupLayoutDescriptor&) const = 0;
            virtual std::expected<BindGroup, std::error_code>       create_bind_group(const BindGroupDescriptor&) const              = 0;

            virtual std::expected<Buffer, std::error_code> create_instance_buffer(std::span<const std::byte>) const                                      = 0;
            virtual std::expected<void, std::error_code>   update_instance_buffer(const Buffer&, std::span<const std::byte>) const                       = 0;
            virtual std::expected<void, std::error_code>   draw_indexed_instanced(const Buffer&, const Buffer&, const Buffer&, uint32_t, uint32_t) const = 0;

            virtual std::expected<std::unique_ptr<occlusion::HiZBuffer>, std::error_code> create_hi_z_buffer(uint32_t, uint32_t) const                                                                                       = 0;
            virtual RenderPass                                                            create_hi_z_pyramid_pass(occlusion::HiZBuffer*, void*) const                                                                       = 0;
            virtual RenderPass                                                            create_hi_z_occlusion_pass(occlusion::HiZBuffer*, const std::vector<occlusion::AABB>*, const glm::mat4&, std::vector<bool>*) const = 0;
            virtual RenderPass                                                            create_instanced_geometry_pass(Buffer, Buffer, Buffer, std::vector<InstanceData>*, std::vector<bool>*, uint32_t) const             = 0;

            virtual void* get_depth_texture() const = 0;
        };

        template <typename T>
        class DeviceModel final : public DeviceConcept
        {
            T _object;

        public:
            DeviceModel(T obj) : _object(std::move(obj))
            {
            }

            std::unique_ptr<DeviceConcept> clone() const override
            {
                return std::make_unique<DeviceModel<T>>(_object);
            }

            bool supports(std::type_index ti) const override
            {
                if (ti == std::type_index(typeid(capabilities::BufferOps)))
                {
                    if constexpr (requires { _object.create_vertex_buffer(std::span<const std::byte>{}); } || requires { _object->create_vertex_buffer(std::span<const std::byte>{}); })
                        return true;
                    return false;
                }
                return true;
            }

            std::expected<Buffer, std::error_code> create_vertex_buffer(std::span<const std::byte> d) const override
            {
                return raktr::render::create_vertex_buffer(_object, d);
            }
            std::expected<Buffer, std::error_code> create_index_buffer(std::span<const std::byte> d) const override
            {
                return raktr::render::create_index_buffer(_object, d);
            }
            std::expected<Buffer, std::error_code> create_uniform_buffer(size_t s) const override
            {
                return raktr::render::create_uniform_buffer(_object, s);
            }
            std::expected<void, std::error_code> update_uniform_buffer(const Buffer& b, std::span<const std::byte> d) const override
            {
                return raktr::render::update_uniform_buffer(_object, b, d);
            }
            void set_uniform_buffer(const Buffer& b) const override
            {
                raktr::render::set_uniform_buffer(_object, b);
            }

            std::expected<void, std::error_code> draw_indexed(const Buffer& vb, const Buffer& ib, uint32_t c) const override
            {
                return raktr::render::draw_indexed(_object, vb, ib, c);
            }
            void clear() const override
            {
                raktr::render::clear(_object);
            }

            std::expected<void, std::error_code> resize(uint32_t w, uint32_t h) const override
            {
                return raktr::render::resize(_object, w, h);
            }
            void set_aspect_ratio(AspectRatio r, float c) const override
            {
                raktr::render::set_aspect_ratio(_object, r, c);
            }
            AspectRatio aspect_ratio() const override
            {
                return raktr::render::aspect_ratio(_object);
            }
            const Viewport& viewport() const override
            {
                return raktr::render::viewport(_object);
            }
            void* get_surface_view() const override
            {
                return raktr::render::get_surface_view(_object);
            }
            void* get_depth_view() const override
            {
                return raktr::render::get_depth_view(_object);
            }

            void present() const override
            {
                raktr::render::present(_object);
            }
            std::expected<Queue, std::error_code> queue() const override
            {
                return raktr::render::queue(_object);
            }
            std::expected<CommandEncoder, std::error_code> create_command_encoder(std::string_view l) const override
            {
                return raktr::render::create_command_encoder(_object, l);
            }

            std::expected<ShaderModule, std::error_code> create_shader_module(const ShaderModuleDescriptor& d) const override
            {
                return raktr::render::create_shader_module(_object, d);
            }
            std::expected<RenderPipeline, std::error_code> create_render_pipeline(const RenderPipelineDescriptor& d) const override
            {
                return raktr::render::create_render_pipeline(_object, d);
            }
            std::expected<ComputePipeline, std::error_code> create_compute_pipeline(const ComputePipelineDescriptor& d) const override
            {
                return raktr::render::create_compute_pipeline(_object, d);
            }

            std::expected<BindGroupLayout, std::error_code> create_bind_group_layout(const BindGroupLayoutDescriptor& d) const override
            {
                return raktr::render::create_bind_group_layout(_object, d);
            }
            std::expected<BindGroup, std::error_code> create_bind_group(const BindGroupDescriptor& d) const override
            {
                return raktr::render::create_bind_group(_object, d);
            }

            std::expected<Buffer, std::error_code> create_instance_buffer(std::span<const std::byte> d) const override
            {
                return raktr::render::create_instance_buffer(_object, d);
            }
            std::expected<void, std::error_code> update_instance_buffer(const Buffer& b, std::span<const std::byte> d) const override
            {
                return raktr::render::update_instance_buffer(_object, b, d);
            }
            std::expected<void, std::error_code> draw_indexed_instanced(const Buffer& vb, const Buffer& ib, const Buffer& kb, uint32_t ic, uint32_t nc) const override
            {
                return raktr::render::draw_indexed_instanced(_object, vb, ib, kb, ic, nc);
            }

            std::expected<std::unique_ptr<occlusion::HiZBuffer>, std::error_code> create_hi_z_buffer(uint32_t w, uint32_t h) const override
            {
                return raktr::render::create_hi_z_buffer(_object, w, h);
            }
            RenderPass create_hi_z_pyramid_pass(occlusion::HiZBuffer* buffer, void* depth) const override
            {
                return raktr::render::create_hi_z_pyramid_pass(_object, buffer, depth);
            }
            RenderPass create_hi_z_occlusion_pass(occlusion::HiZBuffer* buffer, const std::vector<occlusion::AABB>* aabbs, const glm::mat4& vp, std::vector<bool>* results) const override
            {
                return raktr::render::create_hi_z_occlusion_pass(_object, buffer, aabbs, vp, results);
            }
            RenderPass create_instanced_geometry_pass(Buffer vb, Buffer ib, Buffer instb, std::vector<InstanceData>* cpu_data, std::vector<bool>* visibility, uint32_t index_count) const override
            {
                return raktr::render::create_instanced_geometry_pass(_object, vb, ib, instb, cpu_data, visibility, index_count);
            }

            void* get_depth_texture() const override
            {
                return raktr::render::get_depth_texture(_object);
            }
        };

        std::unique_ptr<DeviceConcept> _pimpl;
    };

    template <typename Capability>
    Capability Device::capability() const
    {
        if constexpr (std::is_same_v<Capability, capabilities::BufferOps>)
        {
            return capabilities::BufferOps{
                .create_vertex_buffer = [this](auto d)
                {
                    return this->create_vertex_buffer(d);
                },
                .create_index_buffer = [this](auto d)
                {
                    return this->create_index_buffer(d);
                },
                .create_uniform_buffer = [this](auto s)
                {
                    return this->create_uniform_buffer(s);
                },
                .update_uniform_buffer = [this](auto b, auto d)
                {
                    return this->update_uniform_buffer(b, d);
                },
                .set_uniform_buffer = [this](auto b)
                {
                    this->set_uniform_buffer(b);
                }
            };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::DrawOps>)
        {
            return capabilities::DrawOps{
                .draw_indexed = [this](auto v, auto i, auto c)
                {
                    return this->draw_indexed(v, i, c);
                },
                .clear = [this]()
                {
                    this->clear();
                }
            };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::ViewportOps>)
        {
            return capabilities::ViewportOps{
                .resize = [this](auto w, auto h)
                {
                    return this->resize(w, h);
                },
                .set_aspect_ratio = [this](auto r, auto c)
                {
                    this->set_aspect_ratio(r, c);
                },
                .aspect_ratio = [this]()
                {
                    return this->aspect_ratio();
                },
                .viewport = [this]()
                {
                    return this->viewport();
                },
                .get_surface_view = [this]()
                {
                    return this->get_surface_view();
                },
                .get_depth_view = [this]()
                {
                    return this->get_depth_view();
                }
            };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::PresentOps>)
        {
            return capabilities::PresentOps{
                .present = [this]()
                {
                    this->present();
                }
            };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::QueueOps>)
        {
            return capabilities::QueueOps{ .queue = [this]()
                                           {
                                               return this->queue();
                                           } };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::CommandEncoderOps>)
        {
            return capabilities::CommandEncoderOps{ .create_command_encoder = [this](auto l)
                                                    {
                                                        return this->create_command_encoder(l);
                                                    } };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::ShaderOps>)
        {
            return capabilities::ShaderOps{ .create_shader_module = [this](auto d)
                                            {
                                                return this->create_shader_module(d);
                                            } };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::RenderPipelineOps>)
        {
            return capabilities::RenderPipelineOps{ .create_render_pipeline = [this](auto d)
                                                    {
                                                        return this->create_render_pipeline(d);
                                                    } };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::ComputePipelineOps>)
        {
            return capabilities::ComputePipelineOps{ .create_compute_pipeline = [this](auto d)
                                                     {
                                                         return this->create_compute_pipeline(d);
                                                     } };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::BindGroupOps>)
        {
            return capabilities::BindGroupOps{
                .create_bind_group_layout = [this](auto d)
                {
                    return this->create_bind_group_layout(d);
                },
                .create_bind_group = [this](auto d)
                {
                    return this->create_bind_group(d);
                }
            };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::InstancingOps>)
        {
            return capabilities::InstancingOps{
                .create_instance_buffer = [this](auto d)
                {
                    return this->create_instance_buffer(d);
                },
                .update_instance_buffer = [this](auto b, auto d)
                {
                    return this->update_instance_buffer(b, d);
                },
                .draw_indexed_instanced = [this](auto v, auto i, auto k, auto ic, auto n)
                {
                    return this->draw_indexed_instanced(v, i, k, ic, n);
                }
            };
        }
        else if constexpr (std::is_same_v<Capability, capabilities::OcclusionCullingOps>)
        {
            return capabilities::OcclusionCullingOps{
                .create_hi_z_buffer = [this](auto w, auto h)
                {
                    return this->create_hi_z_buffer(w, h);
                },
                .get_depth_texture = [this]()
                {
                    return this->get_depth_texture();
                }
            };
        }

        throw std::bad_optional_access();
    }

    using DeviceView = Device;
} // namespace raktr::render

#endif
