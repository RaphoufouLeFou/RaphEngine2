#pragma once

#include <RaphEngine2/export.hpp>
#include <string>
#include <type_traits>
#include "Serializable.hpp"

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
        std::is_convertible<T, Serializable>();
    };
    
    template<SettingsValue T>
    struct RAPHENGINE_API Settings : public Serializable
    {
        Settings(const std::string& name_)
            : name(name_)
        {}

        Settings(const std::string& name_, T default_)
            : name(name_)
            , value(default_)
        {}

        std::string serialize() override;
        bool deserialize(const std::string& input) override;

        const std::string name;
        T value;
    };
}
