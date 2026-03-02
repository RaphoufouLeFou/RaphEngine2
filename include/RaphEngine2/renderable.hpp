#pragma once

#include <RaphEngine2/export.hpp>

namespace raphEngine::objects
{
    class RAPHENGINE_API Renderable
    {
        virtual void render() = 0;
    };
}
