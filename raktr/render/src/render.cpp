#include "render.h"

namespace raktr::render
{
    struct Render::Impl
    {
        // Implementation details here
    };

    Render::Render()
        : impl_(std::make_unique<Impl>())
    {
    }

    Render::~Render() = default;

    void Render::init()
    {
        // Initialization logic here
    }

    void Render::shutdown()
    {
        // Shutdown logic here
    }

    std::unique_ptr<Render> make()
    {
        return std::make_unique<Render>();
    }
} // namespace raktr::render