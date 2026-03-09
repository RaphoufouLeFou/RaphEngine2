#include "RaphEngine2/settings/graphics.hpp"

#include "nlohmann/json_fwd.hpp"
#include "settings/settings.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

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

    Graphics::Api Graphics::getApi() const
    {
        return api_.value;
    }
    Graphics::Quality Graphics::getShadowQuality() const
    {
        return shadow_.value;
    }
    bool Graphics::getFullScreen() const
    {
        return fullscreen_.value;
    }
    std::pair<unsigned short, unsigned short> Graphics::getResolution() const
    {
        const std::string res = resolution_.value;
        size_t index = res.find('x');
        if (index == std::string::npos)
            return std::pair<unsigned short, unsigned short>{1920, 1080};

        std::string res_x = res.substr(0, index);
        std::string res_y = res.substr(index + 1);

        unsigned short int_x = std::stoi(res_x);
        unsigned short int_y = std::stoi(res_y);

        return std::pair<unsigned short, unsigned short>{int_x, int_y};
    }

    void Graphics::setApi(Api new_api)
    {
        api_.value = new_api;
    }
    void Graphics::setShadowQuality(Quality new_shadow)
    {
        shadow_.value = new_shadow;
    }
    void Graphics::setFullSceen(bool new_fullscreen)
    {
        fullscreen_.value = new_fullscreen;
    }
    void Graphics::setResolution(unsigned short new_x, unsigned short new_y)
    {
        std::stringstream ss{};
        ss << new_x << 'x' << new_y;
        resolution_.value = ss.str();
    }

} // namespace raphEngine::settings
