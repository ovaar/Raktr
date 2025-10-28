/*!
 * @file render.h
 * @brief Raktr Render global context class definition.
 */

#ifndef RAKTR_RENDER_RENDER_H
#define RAKTR_RENDER_RENDER_H

#include <memory>

namespace raktr::render
{
    /*!
     * @brief Raktr Render global context class.
     */
    class Render
    {
    public:
        Render();
        ~Render();

    private:
        void init();
        void shutdown();

        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    /*!
     * @brief Raktr Render initialization guard.
     */
    class RenderInitGuard
    {
    private:
        friend class Render;
        std::weak_ptr<Render> _render;

    public:
        RenderInitGuard(std::weak_ptr<Render> render)
            : _render(render)
        {
            if (auto rend = _render.lock())
            {
                rend->init();
            }
        }
        ~RenderInitGuard()
        {
            if (auto rend = _render.lock())
            {
                rend->shutdown();
            }
        }
    };

    std::unique_ptr<Render> make();
} // namespace raktr::render

#endif // RAKTR_RENDER_RENDER_H