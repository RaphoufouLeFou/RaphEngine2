#pragma once

#include <RaphEngine2/export.hpp>

#include "renderable.hpp"
#include "settings/graphics.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GraphicApi
    {
    public:
        virtual void Init(const settings::Graphics& graphics_settings,
                          const std::string& window_name) = 0;
        virtual void Render() = 0;
        virtual void AddToRenderPool(const Renderable& renderable) = 0;
        virtual void Refresh() = 0;
    };
} // namespace raphEngine::graphics
