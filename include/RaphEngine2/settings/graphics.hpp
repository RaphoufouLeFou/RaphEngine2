#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "generic_settings.hpp"
#include "settings.hpp"
#include "settings/save_settings.hpp"

namespace raphEngine::settings
{

    class RAPHENGINE_API Graphics : public SavableSetting
    {
    public:
        nlohmann::json serialize() const override;
        bool deserialize(const nlohmann::json& input) override;

        const std::string get_settings_name() const override
        {
            return "Graphics";
        }

        enum class Api
        {

            OPENGL = 0,
            VULAKAN,
            DX11,
        };

        constexpr static std::array<std::string_view, 3> api_names = {
            "openGL",
            "Vulkan",
            "DX11",
        };

        enum class Quality
        {
            HIGH,
            MEDIUM,
            LOW,
        };

        constexpr static std::array<std::string_view, 3> quality_names = {
            "High",
            "Medium",
            "Low",
        };

        Api getApi() const;
        Quality getShadowQuality() const;
        bool getFullScreen() const;
        std::pair<unsigned short, unsigned short> getResolution() const;

        void setApi(Api new_api);
        void setShadowQuality(Quality new_shadow);
        void setFullSceen(bool new_fullscreen);
        void setResolution(unsigned short new_x, unsigned short new_y);

    private:
        EnumSetting<Api, 3> api_ = EnumSetting<Api, 3>("Api", api_names);
        EnumSetting<Quality, 3> shadow_ =
            EnumSetting<Quality, 3>("Shadow", quality_names);
        BooleanSetting fullscreen_ = BooleanSetting("Fullscreen", false);
        StringSetting resolution_ = StringSetting("Resolution", "1920x1080");

        std::vector<Settings*> graphics_settings = {
            &api_,
            &shadow_,
            &fullscreen_,
            &resolution_,
        };

        static bool registered;
    };
} // namespace raphEngine::settings
