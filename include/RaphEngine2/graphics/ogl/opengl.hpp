#pragma once

#include <RaphEngine2/export.hpp>
#include "graphics/graphic_api.hpp"

namespace raphEngine::graphics::ogl {
    class RAPHENGINE_API OpenGL : public GraphicApi
    {
    public:
        void Init(const settings::Graphics& graphics_settings, const std::string& window_name) override;
        void Render(const Renderable& renderable) override;
    };
}
