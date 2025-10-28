#include "engine.h"

namespace raktr::engine
{
    struct Engine::Impl
    {
        // Implementation details here
    };

    Engine::Engine()
        : _impl(std::make_unique<Impl>())
    {
    }

    Engine::~Engine() = default;

    void Engine::init()
    {
        // Initialization logic here
    }

    void Engine::shutdown()
    {
        // Shutdown logic here
    }

    std::unique_ptr<Engine> make()
    {
        return std::make_unique<Engine>();
    }
} // namespace raktr::engine