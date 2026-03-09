#include "RaphEngine2/settings/generic_settings.hpp"
#include "nlohmann/json_fwd.hpp"
#include "settings/settings.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace raphEngine::settings
{
    void BooleanSetting::add_to_json(nlohmann::json& parent_node)const
    {
        parent_node[get_pretty_name()] = value;
    }

    void BooleanSetting::from_json(const nlohmann::json& parent_node)
    {
        value = parent_node.at(get_pretty_name()).get<bool>();
    }

    void IntegerSetting::add_to_json(nlohmann::json& parent_node)const
    {
        parent_node[get_pretty_name()] = value;
    }

    void IntegerSetting::from_json(const nlohmann::json& parent_node)
    {
        value = parent_node.at(get_pretty_name()).get<int>();
    }

    void StringSetting::add_to_json(nlohmann::json& parent_node)const
    {
        parent_node[get_pretty_name()] = value;
    }

    void StringSetting::from_json(const nlohmann::json& parent_node)
    {
        value = parent_node.at(get_pretty_name()).get<std::string>();
    }

} // namespace raphEngine::settings
