
#include "settings/save_settings.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <vector>
#include "settings/graphics.hpp"
#include "settings/settings.hpp"

namespace raphEngine::settings
{
    void SettingsSaver::save_settings(std::vector<SavableSetting*>& settings, std::filesystem::path path)
    {
        std::ofstream ofs(path);
        if(!ofs.is_open())
            throw std::invalid_argument(std::string("can't open file \"") + path.c_str() + "\"");
        std::cout << "saving " << settings.size() << " setting(s) to " << path << "\n";
        nlohmann::json arr = nlohmann::json::array();
        for (auto& setting : settings)
        {
            nlohmann::json node;
            node[setting->get_settings_name()] = setting->serialize();
            arr.push_back(node);
        }
        ofs << arr.dump(4);
    }

    std::vector<std::unique_ptr<SavableSetting>> load_default()
    {
        std::vector<std::unique_ptr<SavableSetting>> res;
        res.push_back(std::make_unique<Graphics>());
        return res;
    }

    std::vector<std::unique_ptr<SavableSetting>> SettingsSaver::load_settings(std::filesystem::path path)
    {
        std::ifstream ifs(path);

        if(!ifs.is_open() || true)
            return load_default();

        nlohmann::json parent;
        ifs >> parent;
        return load_default();
    }
}
