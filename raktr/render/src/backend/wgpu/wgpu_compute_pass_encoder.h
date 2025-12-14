/*!
 * @file wgpu_compute_pass_encoder.h
 * @brief WebGPU compute pass encoder implementation.
 */

#ifndef RAKTR_RENDER_WGPU_COMPUTE_PASS_H
#define RAKTR_RENDER_WGPU_COMPUTE_PASS_H

#include <webgpu/webgpu.h>

namespace raktr::render::backend
{
    /*!
     * @brief WebGPU compute pass encoder implementation.
     *
     * Wraps WGPUComputePassEncoder for recording compute commands.
     */
    class WgpuComputePassEncoder
    {
    public:
        /*!
         * @brief Construct from WebGPU compute pass encoder handle.
         * @param encoder WebGPU compute pass encoder (takes ownership).
         */
        explicit WgpuComputePassEncoder(WGPUComputePassEncoder encoder)
            : _encoder(encoder)
        {
        }

        // Non-copyable
        WgpuComputePassEncoder(const WgpuComputePassEncoder&)            = delete;
        WgpuComputePassEncoder& operator=(const WgpuComputePassEncoder&) = delete;

        // Movable
        WgpuComputePassEncoder(WgpuComputePassEncoder&& other) noexcept
            : _encoder(other._encoder)
        {
            other._encoder = nullptr;
        }

        WgpuComputePassEncoder& operator=(WgpuComputePassEncoder&& other) noexcept
        {
            if (this != &other)
            {
                cleanup();
                _encoder       = other._encoder;
                other._encoder = nullptr;
            }
            return *this;
        }

        ~WgpuComputePassEncoder()
        {
            cleanup();
        }

        /*!
         * @brief Dispatch compute workgroups.
         * @param workgroup_count_x Number of workgroups in X dimension.
         * @param workgroup_count_y Number of workgroups in Y dimension.
         * @param workgroup_count_z Number of workgroups in Z dimension.
         */
        void dispatch(uint32_t workgroup_count_x, uint32_t workgroup_count_y, uint32_t workgroup_count_z) const;

        /*!
         * @brief End the compute pass.
         */
        void end() const;

        /*!
         * @brief Get the underlying WebGPU compute pass encoder.
         * @return WGPUComputePassEncoder handle.
         */
        [[nodiscard]] WGPUComputePassEncoder wgpu_encoder() const
        {
            return _encoder;
        }

    private:
        void cleanup()
        {
            if (_encoder)
            {
                // Note: Don't release here - compute pass encoder is ended, not released
                // It's owned by the command encoder
                _encoder = nullptr;
            }
        }

        mutable WGPUComputePassEncoder _encoder; // Mutable for recording
    };

} // namespace raktr::render::backend

#endif // RAKTR_RENDER_WGPU_COMPUTE_PASS_H
