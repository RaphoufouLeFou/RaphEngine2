#include "RaphEngine2/settings/graphics.hpp"

#include "settings/settings.hpp"

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

    Settings<Graphics::Api>& Graphics::get_api_mode()
    {
        return api_;
    }

    Settings<Quality>& Graphics::get_shadow_quality()
    {
        return shadow_;
    }
} // namespace raphEngine::settings