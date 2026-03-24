#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>
#include <string>
#include <type_traits>
#include "serializable.hpp"
#include "settings/save_settings.hpp"
#include <nlohmann/json.hpp>
#include <vector>

namespace raphEngine::settings {

    class RAPHENGINE_API Settings
    {
    public:
        Settings(const std::string& setting_name)
            : setting_name_ {setting_name}
        {}

        const std::string& get_pretty_name() const;

        virtual void add_to_json(nlohmann::json& parent_node) const = 0;
        virtual void from_json(const nlohmann::json& parent_node) = 0;
        
        const std::string setting_name_;
    };
}
