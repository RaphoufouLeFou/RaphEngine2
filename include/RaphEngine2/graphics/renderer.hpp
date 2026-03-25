#pragma once

#include <RaphEngine2/export.hpp>

#include "settings/graphics.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API Renderer
    {
    public:
        virtual void render() = 0;
    };
} // namespace raphEngine::graphics
