#include "RaphEngine2/settings/graphics.hpp"

#include "nlohmann/json_fwd.hpp"
#include "settings/settings.hpp"
#include <nlohmann/json.hpp>

namespace raphEngine::settings
{
    
    bool Graphics::registered =
    (SettingsSaver::register_setting("Graphics", [](){
        return std::make_unique<Graphics>();
    }), true);

    nlohmann::json Graphics::serialize() const
    {
        nlohmann::json node;
        for (const auto& s : graphics_settings)
        {
            s->add_to_json(node);
        }
        return node;
    }
    bool Graphics::deserialize(const nlohmann::json& input)
    {
        for (const auto& s : graphics_settings)
        {
            try
            {
                s->from_json(input);
            }
            catch(...)
            {
                return false;
            }
        }
        return true;
    }

} // namespace raphEngine::settings
