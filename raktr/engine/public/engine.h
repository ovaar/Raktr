/*!
 * @file engine.h
 * @brief Raktr Engine global context class definition.
 */

#ifdef RAKTR_ENGINE_ENGINE_H
#define RAKTR_ENGINE_ENGINE_H

#include <memory>

namespace raktr::engine
{
    /*!
     * @brief Raktr Engine global context class.
     */
    class Engine
    {
    public:
        Engine();
        ~Engine();

    private:
        void init();
        void shutdown();

        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    /*!
     * @brief Raktr Engine initialization guard.
     */
    class EngineInitGuard
    {
    private:
        friend class Engine;
        std::weak_ptr<Engine> engine_;

    public:
        EngineInitGuard(std::weak_ptr<Engine> engine)
            : engine_(engine)
        {
            if (auto eng = engine_.lock())
            {
                eng->init();
            }
        }
        ~EngineInitGuard()
        {
            if (auto eng = engine_.lock())
            {
                eng->shutdown();
            }
        }
    };

    std::unique_ptr<Engine> make();
} // namespace raktr::engine

#endif // RAKTR_ENGINE_ENGINE_H