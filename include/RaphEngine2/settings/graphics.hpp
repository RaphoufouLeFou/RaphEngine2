#pragma once

#include <RaphEngine2/export.hpp>
#include "settings.hpp"
#include "settings/save_settings.hpp"
#include "graphics/resolution.hpp"
#include <nlohmann/json.hpp>

namespace raphEngine::settings {

    class RAPHENGINE_API Graphics : public SavableSetting
    {
    public:
        enum class Api{
    
            OPENGL,
            // TODO:
            // VULAKAN
            // DX11/12 ?
        };

    nlohmann::json serialize() const override;
    bool deserialize(const nlohmann::json& input) override;

    constexpr std::string get_settings_name() const override
    {
        return "Graphics";
    }

    void set_api(Api api_mode);
    void set_shadow(Quality shadow_quality);
    void set_resolution(graphics::Resolution resolution);
    void set_fullscreen(bool fullscreen);
    

    Settings<Api>& get_api_mode();
    Settings<Quality>& get_shadow_quality();
    Settings<graphics::Resolution>& get_resolution();
    Settings<bool>& get_fullscreen();
    
    private:
        Settings<Graphics::Api> api_ = Settings<Graphics::Api>("Graphics API", Api::OPENGL);
        Settings<Quality> shadow_ = Settings<Quality>("Shadow quality", Quality::HIGH);
        Settings<graphics::Resolution> resolution_ = Settings<graphics::Resolution>("Resolution");
        Settings<bool> fullscreen_ = Settings<bool>("Fullscreen mode", true);
    };
}
