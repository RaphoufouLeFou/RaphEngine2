
#include "settings/save_settings.hpp"
#include <fstream>
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <vector>

namespace raphEngine::settings
{
    void SettingsSaver::save_settings(std::vector<SavableSetting*>& settings, std::filesystem::path path)
    {
        std::ofstream ofs(path);
        if(!ofs.is_open())
            throw std::invalid_argument(std::string("can't open file \"") + path.c_str() + "\"");
            
        nlohmann::json arr = nlohmann::json::array();
        for (auto& setting : settings)
        {
            nlohmann::json node;
            node[setting->get_settings_name()] = setting->serialize();
            arr.push_back(node);
        }
    }

    std::vector<std::unique_ptr<SavableSetting>> SettingsSaver::load_settings(std::filesystem::path path)
    {
        std::vector<std::unique_ptr<SavableSetting>> res;
        (void) path;
        return res;
    }
}
