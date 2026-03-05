#include "RaphEngine2/settings/graphics.hpp"

#include "nlohmann/json_fwd.hpp"
#include "settings/settings.hpp"
#include <nlohmann/json.hpp>

namespace raphEngine::settings
{

    void Graphics::set_api(Api api_mode)
    {
        api_.value = api_mode;
    }
    void Graphics::set_shadow(Quality shadow_quality)
    {
        shadow_.value = shadow_quality;
    }
    
    void Graphics::set_resolution(graphics::Resolution resolution)
    {
        resolution_.value = resolution;
    }
    void Graphics::set_fullscreen(bool fullscreen)
    {
        fullscreen_.value = fullscreen;
    }
    
    Settings<Graphics::Api>& Graphics::get_api_mode()
    {
        return api_;
    }

    Settings<Quality>& Graphics::get_shadow_quality()
    {
        return shadow_;
    }

    Settings<graphics::Resolution>& Graphics::get_resolution()
    {
        return resolution_;
    }

    Settings<bool>& Graphics::get_fullscreen()
    {
        return fullscreen_;
    }

    nlohmann::json Graphics::serialize() const
    {
        nlohmann::json arr = nlohmann::json::array();

        arr.push_back(api_.serialize());
        arr.push_back(shadow_.serialize());
        arr.push_back(resolution_.serialize());
        arr.push_back(fullscreen_.serialize());

        return arr;
    }
    bool Graphics::deserialize(const nlohmann::json& input)
    {
        (void) input;
        return true;
    }
} // namespace raphEngine::settings