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
#include "device_capabilities.h"
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
     * This class wraps any concrete device type (WgpuDevice, FakeDevice, etc.)
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
         * @brief Construct a Device from any concrete device type.
         * @param device_impl Concrete device instance (WgpuDevice, FakeDevice, etc.).
         */
        template <typename T>
        Device(T device_impl)
            : _impl(std::make_unique<Model<T>>(std::move(device_impl)))
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
            [[nodiscard]] virtual std::optional<capabilities::BufferOps>     do_capability_bufferops() const     = 0;
            [[nodiscard]] virtual std::optional<capabilities::DrawOps>       do_capability_drawops() const       = 0;
            [[nodiscard]] virtual std::optional<capabilities::ViewportOps>   do_capability_viewportops() const   = 0;
            [[nodiscard]] virtual std::optional<capabilities::PresentOps>    do_capability_presentops() const    = 0;
            [[nodiscard]] virtual std::optional<capabilities::InstancingOps> do_capability_instancingops() const = 0;

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
                return std::nullopt;
            }
        };

        /*!
         * @brief Model implementation wrapping concrete device type T.
         */
        template <typename T>
        struct Model : Concept
        {
            explicit Model(T device_impl)
                : _device(std::move(device_impl))
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
            mutable T                                     _device;
            std::unordered_map<std::type_index, std::any> _capabilities;

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
            }

            std::optional<capabilities::BufferOps> do_capability_bufferops() const override
            {
                if constexpr (requires(T& d, std::span<const std::byte> data, size_t sz, const Buffer& buf) {
                                  { d.create_vertex_buffer(data) } -> std::same_as<std::expected<Buffer, std::error_code>>;
                              })
                {
                    capabilities::BufferOps ops;
                    ops.create_vertex_buffer = [dev = &_device](std::span<const std::byte> data) mutable
                    {
                        return dev->create_vertex_buffer(data);
                    };
                    ops.create_index_buffer = [dev = &_device](std::span<const std::byte> data) mutable
                    {
                        return dev->create_index_buffer(data);
                    };
                    ops.create_uniform_buffer = [dev = &_device](size_t sz) mutable
                    {
                        return dev->create_uniform_buffer(sz);
                    };
                    ops.update_uniform_buffer = [dev = &_device](const Buffer& buf, std::span<const std::byte> data) mutable
                    {
                        return dev->update_uniform_buffer(buf, data);
                    };
                    ops.set_uniform_buffer = [dev = &_device](const Buffer& buf) mutable
                    {
                        dev->set_uniform_buffer(buf);
                    };
                    return ops;
                }
                return std::nullopt;
            }

            std::optional<capabilities::DrawOps> do_capability_drawops() const override
            {
                if constexpr (requires(T& d, const Buffer& vb, const Buffer& ib, uint32_t count) {
                                  { d.draw_indexed(vb, ib, count) } -> std::same_as<std::expected<void, std::error_code>>;
                              })
                {
                    capabilities::DrawOps ops;
                    ops.draw_indexed = [dev = &_device](const Buffer& vb, const Buffer& ib, uint32_t count) mutable
                    {
                        return dev->draw_indexed(vb, ib, count);
                    };
                    ops.clear = [dev = &_device]() mutable
                    {
                        dev->clear();
                    };
                    return ops;
                }
                return std::nullopt;
            }

            std::optional<capabilities::ViewportOps> do_capability_viewportops() const override
            {
                if constexpr (requires(T& d, uint32_t w, uint32_t h, AspectRatio ar, float custom) {
                                  { d.resize(w, h) } -> std::same_as<std::expected<void, std::error_code>>;
                              })
                {
                    capabilities::ViewportOps ops;
                    ops.resize = [dev = &_device](uint32_t w, uint32_t h) mutable
                    {
                        return dev->resize(w, h);
                    };
                    ops.set_aspect_ratio = [dev = &_device](AspectRatio ar, float custom) mutable
                    {
                        dev->set_aspect_ratio(ar, custom);
                    };
                    ops.aspect_ratio = [dev = &_device]() mutable
                    {
                        return dev->aspect_ratio();
                    };
                    ops.viewport = [dev = &_device]() mutable -> const Viewport&
                    {
                        return dev->viewport();
                    };
                    return ops;
                }
                return std::nullopt;
            }

            std::optional<capabilities::PresentOps> do_capability_presentops() const override
            {
                if constexpr (requires(T& d) {
                                  { d.present() } -> std::same_as<void>;
                              })
                {
                    capabilities::PresentOps ops;
                    ops.present = [dev = &_device]() mutable
                    {
                        dev->present();
                    };
                    return ops;
                }
                return std::nullopt;
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
                    ops.create_instance_buffer = [dev = &_device](std::span<const std::byte> data) mutable
                    {
                        return dev->create_instance_buffer(data);
                    };
                    ops.update_instance_buffer = [dev = &_device](const Buffer& buffer, std::span<const std::byte> data) mutable
                    {
                        return dev->update_instance_buffer(buffer, data);
                    };
                    ops.draw_indexed_instanced = [dev = &_device](const Buffer& vertex_buffer,
                                                                  const Buffer& index_buffer,
                                                                  const Buffer& instance_buffer,
                                                                  uint32_t      index_count,
                                                                  uint32_t      instance_count) mutable
                    {
                        return dev->draw_indexed_instanced(vertex_buffer, index_buffer, instance_buffer, index_count, instance_count);
                    };
                    return ops;
                }
                return std::nullopt;
            }
        };

        std::unique_ptr<Concept> _impl;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_DEVICE_H
