#include "settings/settings.hpp"

namespace raphEngine::settings
{
    Settings::~Settings() = default;
    
    std::unordered_map<std::string, Settings::Factory>& Settings::registry()
    {
        static std::unordered_map<std::string, Factory> instance;
        return instance;
    }

    void Settings::register_setting(const std::string& name, Factory factory)
    {
        registry()[name] = factory;
    }

    std::vector<std::unique_ptr<Settings>> Settings::parse_settings(nlohmann::json parent_node)
    {    
        std::vector<std::unique_ptr<Settings>> result;

        for (auto& [key, factory] : registry())
        {
            if (parent_node.contains(key))
            {
                auto setting = factory();
                setting->from_json(parent_node);
                result.push_back(std::move(setting));
            }
        }
        return result;
    }    
}
