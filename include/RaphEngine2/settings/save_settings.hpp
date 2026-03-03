#pragma once

#include <RaphEngine2/export.hpp>
#include "settings.hpp"

namespace raphEngine::settings {

    class RAPHENGINE_API SavableSetting
    {
        virtual std::string serialize() = 0;
        virtual bool deserialize(const std::string& input) = 0;
    };

    class RAPHENGINE_API SettingsSaver
    {
        
    };
}