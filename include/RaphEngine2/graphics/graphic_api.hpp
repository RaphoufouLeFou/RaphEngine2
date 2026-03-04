#pragma once

#include <RaphEngine2/export.hpp>
#include "renderable.hpp"
#include "settings/graphics.hpp"

namespace raphEngine::graphics {
    class RAPHENGINE_API GraphicApi
    {
        virtual void Init(const settings::Graphics& graphics_settings, const std::string& window_name) = 0;
        virtual void Render(const Renderable& renderable) = 0;
    };
}
