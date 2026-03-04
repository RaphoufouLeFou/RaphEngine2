#pragma once

#include <RaphEngine2/export.hpp>
#include "graphics/graphic_api.hpp"

namespace raphEngine::graphics::ogl {
    class RAPHENGINE_API OpenGL : GraphicApi
    {
        void Init(const settings::Graphics& graphics_settings) override;
    };
}