/*!
 * @file wgpu_hi_z_buffer.h
 * @brief WebGPU implementation of Hierarchical Z-Buffer occlusion culling.
 */
#pragma once

#include "occlusion/hi_z_buffer.h"
#include <vector>
#include <webgpu/webgpu.h>

namespace raktr::render::backend::webgpu
{

    /*!
     * @brief WebGPU implementation of Hi-Z occlusion culling.
     *
     * Uses compute shaders to build depth pyramid and test AABB visibility.
     */
    class WgpuHiZBuffer final : public occlusion::HiZBuffer
    {
    public:
        /*!
         * @brief Construct Hi-Z buffer for given resolution.
         * @param instance WebGPU instance handle (for callback polling).
         * @param device WebGPU device handle.
         * @param queue WebGPU queue handle.
         * @param width Depth buffer width.
         * @param height Depth buffer height.
         */
        WgpuHiZBuffer(WGPUInstance instance, WGPUDevice device, WGPUQueue queue, uint32_t width, uint32_t height);

        ~WgpuHiZBuffer() override;

        // Non-copyable, movable
        WgpuHiZBuffer(const WgpuHiZBuffer&)            = delete;
        WgpuHiZBuffer& operator=(const WgpuHiZBuffer&) = delete;
        WgpuHiZBuffer(WgpuHiZBuffer&&) noexcept;
        WgpuHiZBuffer& operator=(WgpuHiZBuffer&&) noexcept;

        friend void swap(WgpuHiZBuffer& first, WgpuHiZBuffer& second) noexcept;

        [[nodiscard]] std::expected<void, std::error_code>
        build_pyramid(void* depth_texture) override;

        [[nodiscard]] std::expected<std::vector<bool>, std::error_code>
        test_visibility(std::span<const occlusion::AABB> aabbs,
                        const glm::mat4&                 view_projection) override;

        [[nodiscard]] uint32_t mip_levels() const override
        {
            return _mip_levels;
        }
        [[nodiscard]] uint32_t width() const override
        {
            return _width;
        }
        [[nodiscard]] uint32_t height() const override
        {
            return _height;
        }
        [[nodiscard]] Stats stats() const override
        {
            return _stats;
        }

    private:
        void     create_depth_pyramid_pipeline();
        void     create_visibility_test_pipeline();
        void     create_depth_pyramid_texture();
        void     create_depth_copy_pipeline();
        void     initialize_pyramid_to_far_plane();
        void     copy_depth_to_r32float(WGPUCommandEncoder encoder, WGPUTexture depth_texture);
        uint32_t compute_mip_levels(uint32_t width, uint32_t height) const;

        WGPUInstance _instance{ nullptr };
        WGPUDevice   _device{ nullptr };
        WGPUQueue    _queue{ nullptr };

        uint32_t _width{ 0 };
        uint32_t _height{ 0 };
        uint32_t _mip_levels{ 0 };

        // Depth pyramid resources
        WGPUTexture     _depth_pyramid{ nullptr };
        WGPUTextureView _depth_pyramid_view{ nullptr };
        WGPUSampler     _depth_sampler{ nullptr };
        WGPUTexture     _depth_copy{ nullptr }; // R32Float copy of depth texture for compute access

        // Pipelines
        WGPUComputePipeline _pyramid_pipeline{ nullptr };
        WGPUComputePipeline _visibility_pipeline{ nullptr };
        WGPUComputePipeline _depth_copy_pipeline{ nullptr };

        // Bind group layouts
        WGPUBindGroupLayout _pyramid_bind_group_layout{ nullptr };
        WGPUBindGroupLayout _visibility_bind_group_layout{ nullptr };
        WGPUBindGroupLayout _depth_copy_bind_group_layout{ nullptr };

        // Buffers
        WGPUBuffer _aabb_buffer{ nullptr };
        WGPUBuffer _visibility_buffer{ nullptr };
        WGPUBuffer _uniform_buffer{ nullptr };

        Stats _stats{};
    };

} // namespace raktr::render::backend::webgpu
