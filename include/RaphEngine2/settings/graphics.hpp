#pragma once

#include <RaphEngine2/export.hpp>
#include "settings.hpp"
#include "settings/save_settings.hpp"
#include "generic_settings.hpp"
#include <nlohmann/json.hpp>
#include <string>

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
        ApiSetting api_;
        ShadowSetting shadow_;
        FullscreenSetting fullscreen_;
        ResolutionSetting resolution_;
    };
}
