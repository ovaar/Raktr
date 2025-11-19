/*!
 * @file render_pass.h
 * @brief Type-erased render pass using Klaus Iglberger's external polymorphism pattern.
 *
 * This implementation uses type erasure (Concept-Model-Object pattern) to provide
 * runtime polymorphism WITHOUT requiring concrete passes to inherit from a base class.
 *
 * @see Device - Uses the same pattern for type-erased device abstraction
 */

#ifndef RAKTR_RENDER_RENDER_PASS_H
#define RAKTR_RENDER_RENDER_PASS_H

#include <memory>
#include <string_view>
#include <type_traits>

namespace raktr::render
{
    /*!
     * @brief Type-erased render pass using external polymorphism.
     *
     * This class can wrap any concrete pass type without requiring inheritance.
     * Concrete passes only need to implement the required duck-typed interface:
     * - std::string_view name() const
     * - void execute(PassContextType& ctx)
     * - void on_viewport_resize(uint32_t width, uint32_t height)
     *
     * **Benefits over traditional inheritance**:
     * - Non-intrusive: Passes don't need to know about RenderPass wrapper
     * - Type-safe: execute() takes concrete context type (WgpuPassContext, SoftPassContext)
     * - Zero virtual overhead in concrete pass code
     * - Duck typing: Any type with required methods works
     *
     * @example Concrete pass (no inheritance!)
     * class WgpuGeometryPass {
     * public:
     *     using ContextType = WgpuPassContext; // Type trait for context deduction
     *
     *     std::string_view name() const { return "Geometry Pass"; }
     *
     *     void execute(WgpuPassContext& ctx) {
     *         // WebGPU-specific rendering
     *     }
     *
     *     void on_viewport_resize(uint32_t w, uint32_t h) {
     *         // Handle resize
     *     }
     * };
     *
     * @example Usage
     * RenderPass pass = WgpuGeometryPass{};
     * pass.name(); // "Geometry Pass"
     * WgpuPassContext ctx = ...;
     * pass.execute(ctx); // Type-safe call
     */
    class RenderPass
    {
    public:
        /*!
         * @brief Construct from any concrete pass type.
         * @tparam PassType Concrete pass type (e.g., WgpuGeometryPass, SoftGeometryPass).
         * @param pass Concrete pass instance (moved into storage).
         *
         * Requirements (duck typing):
         * - PassType::ContextType typedef must exist
         * - PassType must have: std::string_view name() const
         * - PassType must have: void execute(ContextType& ctx)
         * - PassType must have: void on_viewport_resize(uint32_t, uint32_t)
         */
        template <typename PassType>
        explicit RenderPass(PassType pass)
            : _impl(std::make_unique<Model<PassType>>(std::move(pass)))
        {
        }

        // Non-copyable (some passes may hold GPU resources)
        RenderPass(const RenderPass&)            = delete;
        RenderPass& operator=(const RenderPass&) = delete;

        // Movable
        RenderPass(RenderPass&&) noexcept            = default;
        RenderPass& operator=(RenderPass&&) noexcept = default;

        ~RenderPass() = default;

        /*!
         * @brief Get the human-readable pass name.
         * @return Pass name for debugging/profiling.
         */
        [[nodiscard]] std::string_view name() const
        {
            return _impl->do_name();
        }

        /*!
         * @brief Notify pass of viewport resize.
         * @param width New viewport width in pixels.
         * @param height New viewport height in pixels.
         */
        void on_viewport_resize(uint32_t width, uint32_t height)
        {
            _impl->do_on_viewport_resize(width, height);
        }

        /*!
         * @brief Execute pass with backend-specific context.
         * @tparam PassContextType Backend-specific context (WgpuPassContext, SoftPassContext).
         * @param ctx Backend pass context.
         *
         * **Type Safety**: This method is templated to accept the concrete context type
         * that the wrapped pass expects. If you pass the wrong context type, you'll get
         * a compile-time error.
         *
         * @example
         * WgpuPassContext ctx = ...;
         * wgpu_pass.execute(ctx); // ✅ Type-safe
         * SoftPassContext soft_ctx = ...;
         * wgpu_pass.execute(soft_ctx); // ❌ Compile error!
         */
        template <typename PassContextType>
        void execute(PassContextType& ctx)
        {
            _impl->do_execute(&ctx);
        }

    private:
        /*!
         * @brief Internal polymorphic interface (Concept).
         *
         * This is the abstract base class that enables polymorphism internally.
         * It is an **implementation detail** not exposed to users.
         *
         * **Klaus Iglberger's Pattern**:
         * - Concept: Abstract interface (this struct)
         * - Model<T>: Concrete wrapper (below)
         * - Object: Public wrapper (RenderPass class above)
         */
        struct Concept
        {
            virtual ~Concept()                                                              = default;
            virtual std::string_view do_name() const                                        = 0;
            virtual void             do_on_viewport_resize(uint32_t width, uint32_t height) = 0;
            virtual void             do_execute(void* ctx)                                  = 0; // Type-erased context pointer
        };

        /*!
         * @brief Internal wrapper for concrete pass types (Model).
         *
         * This template class wraps any concrete pass type and forwards
         * calls to it. The concrete pass does NOT need to inherit from anything.
         *
         * **Type Erasure Mechanism**:
         * - execute() receives void* pointer
         * - Casts back to concrete context type using PassType::ContextType
         * - Calls concrete pass's execute() method with typed context
         */
        template <typename PassType>
        struct Model : Concept
        {
            explicit Model(PassType pass)
                : _pass(std::move(pass))
            {
            }

            std::string_view do_name() const override
            {
                return _pass.name();
            }

            void do_on_viewport_resize(uint32_t width, uint32_t height) override
            {
                _pass.on_viewport_resize(width, height);
            }

            void do_execute(void* ctx) override
            {
                // Deduce context type from pass's ContextType typedef
                using ContextType = typename PassType::ContextType;
                _pass.execute(*static_cast<ContextType*>(ctx));
            }

        private:
            PassType _pass; // Wrapped concrete pass (value semantics)
        };

        std::unique_ptr<Concept> _impl; // Type-erased storage (external polymorphism)
    };

} // namespace raktr::render

#endif // RAKTR_RENDER_RENDER_PASS_H
