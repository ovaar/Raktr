/*!
 * @file compute_pass_encoder.h
 * @brief Type-erased compute pass encoder for recording compute commands.
 */

#ifndef RAKTR_RENDER_COMPUTE_PASS_ENCODER_H
#define RAKTR_RENDER_COMPUTE_PASS_ENCODER_H

#include <cstdint>
#include <memory>

namespace raktr::render
{
    /*!
     * @brief Compute pass descriptor.
     */
    struct ComputePassDescriptor
    {
        // Currently no descriptor fields needed
        // Future: timestamp queries, label
    };

    /*!
     * @brief Type-erased compute pass encoder for recording compute commands.
     *
     * ComputePassEncoder is created from CommandEncoder.begin_compute_pass() and provides
     * methods for dispatching compute shaders.
     *
     * The pass must be ended with end() before the command buffer can be finished.
     *
     * @example
     * auto pass = encoder.begin_compute_pass({});
     * pass.dispatch(workgroup_count_x, workgroup_count_y, workgroup_count_z);
     * pass.end();
     */
    class ComputePassEncoder
    {
    public:
        /*!
         * @brief Construct a ComputePassEncoder from any concrete encoder type.
         * @param encoder_impl Concrete encoder instance (WgpuComputePassEncoder, etc.).
         */
        template <typename T>
        ComputePassEncoder(T encoder_impl)
            : _impl(std::make_unique<Model<T>>(std::move(encoder_impl)))
        {
        }

        // Non-copyable
        ComputePassEncoder(const ComputePassEncoder&)            = delete;
        ComputePassEncoder& operator=(const ComputePassEncoder&) = delete;

        // Movable
        ComputePassEncoder(ComputePassEncoder&&) noexcept            = default;
        ComputePassEncoder& operator=(ComputePassEncoder&&) noexcept = default;

        ~ComputePassEncoder() = default;

        /*!
         * @brief Dispatch compute workgroups.
         * @param workgroup_count_x Number of workgroups in X dimension.
         * @param workgroup_count_y Number of workgroups in Y dimension.
         * @param workgroup_count_z Number of workgroups in Z dimension.
         *
         * The total number of shader invocations is:
         * workgroup_count * workgroup_size (defined in shader)
         */
        void dispatch(uint32_t workgroup_count_x, uint32_t workgroup_count_y = 1, uint32_t workgroup_count_z = 1) const
        {
            _impl->do_dispatch(workgroup_count_x, workgroup_count_y, workgroup_count_z);
        }

        /*!
         * @brief End the compute pass.
         *
         * Must be called before finishing the command encoder.
         * After calling end(), this ComputePassEncoder object becomes invalid.
         */
        void end() const
        {
            _impl->do_end();
        }

    private:
        struct Concept
        {
            virtual ~Concept()                                                                                                 = default;
            virtual void do_dispatch(uint32_t workgroup_count_x, uint32_t workgroup_count_y, uint32_t workgroup_count_z) const = 0;
            virtual void do_end() const                                                                                        = 0;
        };

        template <typename T>
        struct Model : Concept
        {
            explicit Model(T encoder_impl)
                : _encoder(std::move(encoder_impl))
            {
            }

            void do_dispatch(uint32_t workgroup_count_x, uint32_t workgroup_count_y, uint32_t workgroup_count_z) const override
            {
                _encoder.dispatch(workgroup_count_x, workgroup_count_y, workgroup_count_z);
            }

            void do_end() const override
            {
                _encoder.end();
            }

            mutable T _encoder;
        };

        std::unique_ptr<Concept> _impl;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_COMPUTE_PASS_ENCODER_H
