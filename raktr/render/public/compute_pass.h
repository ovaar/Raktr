/*!
 * @file compute_pass.h
 * @brief Type-erased compute pass for recording compute commands.
 */

#ifndef RAKTR_RENDER_COMPUTE_PASS_H
#define RAKTR_RENDER_COMPUTE_PASS_H

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
     * @brief Type-erased compute pass for recording compute commands.
     *
     * ComputePass is created from CommandEncoder.begin_compute_pass() and provides
     * methods for dispatching compute shaders.
     *
     * The pass must be ended with end() before the command buffer can be finished.
     *
     * @example
     * auto pass = encoder.begin_compute_pass({});
     * pass.set_pipeline(compute_pipeline);
     * pass.set_bind_group(0, bind_group);
     * pass.dispatch(workgroup_count_x, workgroup_count_y, workgroup_count_z);
     * pass.end();
     */
    class ComputePass
    {
    public:
        /*!
         * @brief Construct a ComputePass from any concrete pass type.
         * @param pass_impl Concrete pass instance (WgpuComputePass, etc.).
         */
        template <typename T>
        ComputePass(T pass_impl)
            : _impl(std::make_unique<Model<T>>(std::move(pass_impl)))
        {
        }

        // Non-copyable
        ComputePass(const ComputePass&)            = delete;
        ComputePass& operator=(const ComputePass&) = delete;

        // Movable
        ComputePass(ComputePass&&) noexcept            = default;
        ComputePass& operator=(ComputePass&&) noexcept = default;

        ~ComputePass() = default;

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
         * After calling end(), this ComputePass object becomes invalid.
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
            explicit Model(T pass_impl)
                : _pass(std::move(pass_impl))
            {
            }

            void do_dispatch(uint32_t workgroup_count_x, uint32_t workgroup_count_y, uint32_t workgroup_count_z) const override
            {
                _pass.dispatch(workgroup_count_x, workgroup_count_y, workgroup_count_z);
            }

            void do_end() const override
            {
                _pass.end();
            }

            mutable T _pass;
        };

        std::unique_ptr<Concept> _impl;
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_COMPUTE_PASS_H
