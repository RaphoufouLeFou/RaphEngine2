#include "graphics/graphic_api.hpp"

#include <RaphEngine2/export.hpp>

namespace raphEngine::graphics
{
    unsigned short GraphicApi::res_x = 640;
    unsigned short GraphicApi::res_y = 480;

    std::vector<const Renderable*> GraphicApi::render_pool;

    
    void GraphicApi::AddToRenderPool(const Renderable* renderable)
    {
        render_pool.push_back(renderable);
    }

} // namespace raphEngine::graphics
