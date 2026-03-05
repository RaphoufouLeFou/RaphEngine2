#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>
#include <string>
#include <type_traits>
#include "nlohmann/json_fwd.hpp"
#include "serializable.hpp"
#include <nlohmann/json.hpp>
#include <vector>

namespace raphEngine::settings {

    enum class Quality
    {
        HIGH,
        MEDIUM,
        LOW,
    };

    enum class SettingType
    {
        STRING,
        BOOL,
        INT,
        QUALITY,
        API,
    };
    
    class RAPHENGINE_API Settings
    {
    public:
        Settings(const std::string& name, const std::string& json_name = "", const std::string& value = "null", SettingType type = SettingType::STRING)
            : type_ {type}
            , json_name_ {json_name}
            , pretty_name_ {name}
            , value_ {value}
        {}
        static std::vector<std::unique_ptr<Settings>> parse_settings(nlohmann::json parent_node);
        static std::vector<std::unique_ptr<Settings>> parse_settings(nlohmann::json parent_node);
    private:
        SettingType type_;
        std::string json_name_;
        std::string pretty_name_;
        std::string value_;
    };
}