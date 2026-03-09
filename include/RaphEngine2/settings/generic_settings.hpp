#pragma once

#include <RaphEngine2/export.hpp>
#include "settings.hpp"
#include "settings/save_settings.hpp"
#include <array>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace raphEngine::settings {

    template<typename T, size_t size>
    class RAPHENGINE_API EnumSetting : public Settings
    {
    public:
        EnumSetting(const std::string& setting_name, const std::array<std::string, size> names, int default_value_index = 0)
            : Settings {setting_name}
            , value { static_cast<T>(default_value_index)}
            , names_ { names }
        {}

        void add_to_json(nlohmann::json& parent_node)const override;
        void from_json(const nlohmann::json& parent_node) override;

        T value;

    protected:
        std::array<std::string, size> names_;
    };

    class RAPHENGINE_API BooleanSetting : public Settings
    {
    public:

        BooleanSetting(const std::string& setting_name, bool default_value = true)
            : Settings {setting_name}
            , value { default_value }
        {}
        void add_to_json(nlohmann::json& parent_node)const override;
        void from_json(const nlohmann::json& parent_node) override;
        
        bool value;
    };

    class RAPHENGINE_API IntegerSetting : public Settings
    {
    public:
    
        IntegerSetting(const std::string& setting_name, long default_value = 0)
            : Settings {setting_name}
            , value { default_value }
        {}

        void add_to_json(nlohmann::json& parent_node)const override;
        void from_json(const nlohmann::json& parent_node) override;
     
        long value;
    };

    class RAPHENGINE_API StringSetting : public Settings
    {
    public:

        StringSetting(const std::string& setting_name, const std::string& default_value = "Null")
            : Settings {setting_name}
            , value { default_value }
        {}

        void add_to_json(nlohmann::json& parent_node)const override;
        void from_json(const nlohmann::json& parent_node) override;

        std::string value;
    };
}

#include "generic_settings.hxx"