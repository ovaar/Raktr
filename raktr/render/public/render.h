/*!
 * @file render.h
 * @brief Raktr Render global context class definition.
 */

#ifdef RAKTR_RENDER_RENDER_H
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
        std::unique_ptr<Impl> impl_;
    };

    /*!
     * @brief Raktr Render initialization guard.
     */
    class RenderInitGuard
    {
    private:
        friend class Render;
        std::weak_ptr<Render> render_;

    public:
        RenderInitGuard(std::weak_ptr<Render> render)
            : render_(render)
        {
            if (auto rend = render_.lock())
            {
                rend->init();
            }
        }
        ~RenderInitGuard()
        {
            if (auto rend = render_.lock())
            {
                rend->shutdown();
            }
        }
    };

    std::unique_ptr<Render> make();
} // namespace raktr::render

#endif // RAKTR_RENDER_RENDER_H