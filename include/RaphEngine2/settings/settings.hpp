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
    
    class RAPHENGINE_API Settings
    {
    public:
        virtual const std::string get_pretty_name() const = 0;

        virtual void add_to_json(nlohmann::json& parent_node);
        

        static std::vector<std::unique_ptr<Settings>> parse_settings(nlohmann::json parent_node);

    };
}