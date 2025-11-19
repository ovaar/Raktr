/*!
 * @file soft_pass_context.h
 * @brief Software renderer pass execution context.
 */

#ifndef RAKTR_RENDER_SOFT_PASS_CONTEXT_H
#define RAKTR_RENDER_SOFT_PASS_CONTEXT_H

#include <cstdint>

namespace raktr::render::backend::soft
{
    // Forward declaration
    class SoftDevice;

    /*!
     * @brief Software renderer pass execution context.
     *
     * Minimal context for software rendering backend (used for testing).
     * Contains only frame state, no GPU resources.
     *
     * @note This is for headless unit testing without GPU.
     */
    struct SoftPassContext
    {
        //! Current frame index
        uint32_t frame_index = 0;

        //! Viewport dimensions
        uint32_t viewport_width  = 0;
        uint32_t viewport_height = 0;

        //! Device access (non-owning reference)
        SoftDevice* device = nullptr;
    };

} // namespace raktr::render::backend::soft

#endif // RAKTR_RENDER_SOFT_PASS_CONTEXT_H
