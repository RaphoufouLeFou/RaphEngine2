#pragma once

#include <RaphEngine2/export.hpp>

#include "RaphEngine2/settings/graphics.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API Renderer
    {
    public:
        virtual void render() = 0;
    };
} // namespace raphEngine::graphics
