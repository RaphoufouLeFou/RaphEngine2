#pragma once

#include <RaphEngine2/export.hpp>
#include <string>
#include <type_traits>

namespace raphEngine::settings {

    enum class Quality
    {
        HIGH,
        MEDIUM,
        LOW,
    };

    template<typename T>
    concept SettingsValue = requires (T value) {
        std::is_default_constructible<T>();
        std::is_move_constructible<T>();
    };
    
    template<SettingsValue T>
    struct RAPHENGINE_API Settings
    {
        Settings(const std::string& name_)
            : name(name_)
        {}

        Settings(const std::string& name_, T default_)
            : name(name_)
            , value(default_)
        {}

        const std::string name;
        T value;
    };
}
