#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include <vector>
#include "graphics/graphic_api.hpp"

namespace raphEngine::graphics::ogl {
    class RAPHENGINE_API OpenGL : public GraphicApi
    {
    public:
        void Init(const settings::Graphics& graphics_settings, const std::string& window_name) override;
        void Render() override;
        void AddToRenderPool(const Renderable& renderable) override;
        void Refresh() override;
    private:
        std::vector<const Renderable*> render_pool;
    };
}
