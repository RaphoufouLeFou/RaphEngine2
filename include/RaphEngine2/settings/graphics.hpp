#pragma once

#include <RaphEngine2/export.hpp>
#include "settings.hpp"
#include "settings/save_settings.hpp"
#include "generic_settings.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include <vector>

namespace raphEngine::settings {

    class RAPHENGINE_API Graphics : public SavableSetting
    {
    public:
        nlohmann::json serialize() const override;
        bool deserialize(const nlohmann::json& input) override;

        constexpr std::string get_settings_name() const override
        {
            return "Graphics";
        }

    private:

        enum class Api{
        
            OPENGL = 0,
            VULAKAN,
            DX11,
        };
        
        constexpr static std::array<std::string, 3> api_names = 
        {
            "openGL",
            "Vulkan",
            "DX11",
        };

        enum class Resolution
        {
            R3840X2160,
            R2560X1440,
            R1920X1080,
            R1280X720,
        };

        constexpr static std::array<std::string, 4> resolution_names = 
        {
            "3840x2160",
            "2560x1440",
            "1920x1080",
            "1280x720",
        };

        enum class Quality
        {
            HIGH,
            MEDIUM,
            LOW,
        };

        constexpr static std::array<std::string, 3> quality_names = 
        {
            "High",
            "Medium",
            "Low",
        }; 

        EnumSetting<Api, 3> api_ = EnumSetting<Api, 3>("Api", api_names);
        EnumSetting<Quality, 3> shadow_ = EnumSetting<Quality, 3>("Shadow", quality_names);
        BooleanSetting fullscreen_ = BooleanSetting("Fullscreen");
        EnumSetting<Resolution, 4> resolution_  = EnumSetting<Resolution, 4>("Resolution", resolution_names);
        
        std::vector<Settings*> graphics_settings = 
        {
            &api_,
            &shadow_,
            &fullscreen_,
            &resolution_,
        };

        static bool registered;
    };
}
