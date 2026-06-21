#pragma once

#include <RaphEngine2/export.hpp>

#include "renderable.hpp"
#include "RaphEngine2/settings/graphics.hpp"
#include "RaphEngine2/objects/mesh.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GraphicApi
    {
    public:
        virtual void Init(const settings::Graphics& graphics_settings,
                          const std::string& window_name) = 0;
        virtual void Render() = 0;
        static void AddToRenderPool(const Renderable* renderable);
        virtual bool Refresh() = 0;

        static unsigned short res_x;
        static unsigned short res_y;

        static std::vector<const Renderable*> render_pool;
    };
} // namespace raphEngine::graphics
