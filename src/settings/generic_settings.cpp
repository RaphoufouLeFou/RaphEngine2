#include "RaphEngine2/settings/generic_settings.hpp"
#include "nlohmann/json_fwd.hpp"
#include "settings/settings.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace raphEngine::settings
{

    template<typename T>
    void EnumSetting<T>::add_to_json(nlohmann::json& parent_node)
    {
        parent_node[get_name()] = get_value_string_at(static_cast<int>(value));
    }

    template<typename T>
    void EnumSetting<T>::from_json(nlohmann::json& parent_node)
    {
        std::string n = parent_node.at(get_name()).template get<std::string>();
        for (size_t i = 0; i < get_enum_size(); i++) {
            if(get_value_string_at(i) == n)
            {
                value = static_cast<T>(i);
            }
        }
    }

    void BooleanSetting::add_to_json(nlohmann::json& parent_node)
    {
        parent_node[get_name()] = value;
    }

    void BooleanSetting::from_json(nlohmann::json& parent_node)
    {
        value = parent_node.at(get_name()).get<bool>();
    }

    void IntegerSetting::add_to_json(nlohmann::json& parent_node)
    {
        parent_node[get_name()] = value;
    }

    void IntegerSetting::from_json(nlohmann::json& parent_node)
    {
        value = parent_node.at(get_name()).get<int>();
    }

    void StringSetting::add_to_json(nlohmann::json& parent_node)
    {
        parent_node[get_name()] = value;
    }

    void StringSetting::from_json(nlohmann::json& parent_node)
    {
        value = parent_node.at(get_name()).get<std::string>();
    }

    // some nightmare that works,
    // to automaticly register a Setting type at compiletime
    // to be used when parsing the json file    
    
    std::string ApiSetting::name = "gAPI";
 
    bool ApiSetting::registered =
    (Settings::register_setting(get_name_static(), [](){
        return std::make_unique<ApiSetting>();
    }), true);

    std::string ShadowSetting::name = "Shadow";
 
    bool ShadowSetting::registered =
    (Settings::register_setting(get_name_static(), [](){
        return std::make_unique<ShadowSetting>();
    }), true);

    std::string FullscreenSetting::name = "FScreen";

    bool FullscreenSetting::registered =
    (Settings::register_setting(get_name_static(), [](){
        return std::make_unique<FullscreenSetting>();
    }), true);

    std::string ResolutionSetting::name = "Res";

    bool ResolutionSetting::registered =
    (Settings::register_setting(get_name_static(), [](){
        return std::make_unique<ResolutionSetting>();
    }), true);

} // namespace raphEngine::settings
