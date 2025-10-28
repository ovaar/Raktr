/*!
 * @file engine.h
 * @brief Raktr Engine global context class definition.
 */

#ifndef RAKTR_ENGINE_ENGINE_H
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

        friend class EngineInitGuard;
        
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    /*!
     * @brief Raktr Engine initialization guard.
     */
    class EngineInitGuard
    {
    private:
        std::weak_ptr<Engine> _engine;

    public:
        EngineInitGuard(std::weak_ptr<Engine> engine)
            : _engine(engine)
        {
            if (auto eng = _engine.lock())
            {
                eng->init();
            }
        }
        ~EngineInitGuard()
        {
            if (auto eng = _engine.lock())
            {
                eng->shutdown();
            }
        }
    };

    std::unique_ptr<Engine> make();
} // namespace raktr::engine

#endif // RAKTR_ENGINE_ENGINE_H