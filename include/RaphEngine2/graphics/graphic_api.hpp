#pragma once

#include <RaphEngine2/export.hpp>
#include "settings/graphics.hpp"

namespace raphEngine::graphics {
    class RAPHENGINE_API GraphicApi
    {
        virtual void Init(const settings::Graphics& graphics_settings) = 0;
    };
}