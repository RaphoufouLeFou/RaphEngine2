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

    class RAPHENGINE_API Settings
    {
    public:
        virtual ~Settings() = default;

        virtual const std::string get_pretty_name() const = 0;

        virtual void add_to_json(nlohmann::json& parent_node);
        virtual void from_json(nlohmann::json& parent_node);

        using Factory = std::function<std::unique_ptr<Settings>()>;
        static void register_setting(const std::string& name, Factory factory);

        static std::vector<std::unique_ptr<Settings>> parse_settings(nlohmann::json parent_node);

    private:
        static std::unordered_map<std::string, Factory>& registry();
    };
}
