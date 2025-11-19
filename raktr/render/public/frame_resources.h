/*!
 * @file frame_resources.h
 * @brief Manages per-frame resources for temporal rendering techniques.
 *
 * FrameResources enables double/triple buffering of GPU resources to support
 * temporal techniques like TAA, temporal occlusion culling, and reprojection.
 * Each frame resource set includes:
 * - Command buffers for async execution
 * - Depth pyramid from previous frame (for temporal occlusion)
 * - Uniform/storage buffers for per-frame data
 *
 * @example
 * @code
 * FrameResources frame_res(device, 2); // double buffering
 *
 * // Render loop
 * auto& current = frame_res.current();
 * // Use current.previous_hi_z_pyramid for occlusion
 * // Build current.hi_z_pyramid for next frame
 *
 * frame_res.advance_frame(); // swap buffers
 * @endcode
 */

#pragma once

#include <cstdint>
#include <vector>

// Forward declarations
struct WGPUTextureImpl;
typedef WGPUTextureImpl* WGPUTexture;

struct WGPUBufferImpl;
typedef WGPUBufferImpl* WGPUBuffer;

struct WGPUDeviceImpl;
typedef WGPUDeviceImpl* WGPUDevice;

namespace raktr::render
{

    /*!
     * @brief Single frame's worth of GPU resources.
     *
     * Contains resources that change every frame:
     * - Hi-Z pyramid texture (both current and previous)
     * - Per-frame uniform buffers (view/projection matrices, etc.)
     * - Fence/semaphore for synchronization
     */
    struct FrameResource
    {
        WGPUTexture hi_z_pyramid{ nullptr };   //!< Current frame's depth pyramid
        WGPUBuffer  uniform_buffer{ nullptr }; //!< Per-frame uniforms
        uint64_t    fence_value{ 0 };          //!< CPU-GPU sync

        FrameResource()  = default;
        ~FrameResource() = default;

        // Non-copyable (RAII resources)
        FrameResource(const FrameResource&)            = delete;
        FrameResource& operator=(const FrameResource&) = delete;

        // Movable
        FrameResource(FrameResource&&) noexcept            = default;
        FrameResource& operator=(FrameResource&&) noexcept = default;
    };

    /*!
     * @brief Manages multiple FrameResource instances for temporal techniques.
     *
     * Provides ring-buffer access to frame resources:
     * - current(): Frame being rendered now
     * - previous(): Frame rendered last (for temporal techniques)
     * - advance_frame(): Move to next frame in ring buffer
     *
     * @example
     * // Double buffering (2 frames in flight)
     * FrameResources frames(device, 2);
     *
     * while (rendering) {
     *     auto& curr = frames.current();
     *     auto& prev = frames.previous();
     *
     *     // Use prev.hi_z_pyramid for occlusion culling
     *     // Render to curr
     *
     *     frames.advance_frame();
     * }
     */
    class FrameResources
    {
    public:
        /*!
         * @brief Constructs frame resource manager.
         * @param device WebGPU device for resource creation.
         * @param num_frames Number of frames to buffer (2=double, 3=triple).
         * @example
         * FrameResources frames(device, 2); // double buffering
         */
        explicit FrameResources(WGPUDevice device, uint32_t num_frames = 2);

        ~FrameResources();

        // Non-copyable
        FrameResources(const FrameResources&)            = delete;
        FrameResources& operator=(const FrameResources&) = delete;

        // Movable
        FrameResources(FrameResources&&) noexcept            = default;
        FrameResources& operator=(FrameResources&&) noexcept = default;

        /*!
         * @brief Gets the current frame's resources.
         * @return Reference to active FrameResource.
         */
        [[nodiscard]] FrameResource& current();

        /*!
         * @brief Gets the previous frame's resources (for temporal techniques).
         * @return Reference to previous FrameResource.
         * @note For first frame, returns current frame (no temporal data yet).
         */
        [[nodiscard]] const FrameResource& previous() const;

        /*!
         * @brief Advances to next frame in ring buffer.
         * @example
         * // Render loop
         * render_frame(frames.current());
         * frames.advance_frame(); // move to next
         */
        void advance_frame();

        /*!
         * @brief Gets total number of buffered frames.
         * @return Frame count (2 or 3 typically).
         */
        [[nodiscard]] uint32_t num_frames() const
        {
            return static_cast<uint32_t>(_frames.size());
        }

        /*!
         * @brief Gets current frame index.
         * @return Index in [0, num_frames).
         */
        [[nodiscard]] uint32_t current_index() const
        {
            return _current_index;
        }

        /*!
         * @brief Recreates frame resources (e.g., after viewport resize).
         * @param width New texture width.
         * @param height New texture height.
         */
        void recreate_resources(uint32_t width, uint32_t height);

        /*!
         * @brief Waits for GPU to finish using all frame resources.
         * @note Blocks CPU until all in-flight frames complete.
         */
        void wait_idle();

    private:
        WGPUDevice                 _device{ nullptr };
        std::vector<FrameResource> _frames;
        uint32_t                   _current_index{ 0 };
        uint32_t                   _frame_counter{ 0 }; //!< Total frames rendered (for temporal history)
    };

} // namespace raktr::render
