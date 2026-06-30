#include "graphics/graphic_api.hpp"

#include <RaphEngine2/export.hpp>

namespace raphEngine::graphics
{
    unsigned short GraphicApi::res_x = 640;
    unsigned short GraphicApi::res_y = 480;

    std::vector<const Renderable*> GraphicApi::render_pool;
    std::vector<const component::LightComponent*> GraphicApi::lights_pool;
    std::vector<const component::LightComponent*> GraphicApi::spot_lights_pool;

    GraphicApi* GraphicApi::instance_ = nullptr;

    void GraphicApi::AddToRenderPool(const Renderable* renderable)
    {
        render_pool.push_back(renderable);
    }

    void GraphicApi::AddToLightsPool(
        const component::LightComponent* light_componentt)
    {
        lights_pool.push_back(light_componentt);
    }

    void GraphicApi::AddToSpotLightsPool(
        const component::LightComponent* light_componentt)
    {
        spot_lights_pool.push_back(light_componentt);
    }

    const GraphicApi* GraphicApi::get_api()
    {
        return instance_;
    }

} // namespace raphEngine::graphics
