#pragma once

#include <RaphEngine2/export.hpp>
#include "renderer.hpp"
#include "settings/graphics.hpp"

namespace raphEngine::graphics {
    class RAPHENGINE_API MeshRenderer : public Renderer
    {
    public:
        virtual void render() override = 0;
    };
}
