#include "RaphEngine2/settings/settings.hpp"

#include <fstream>
#include <iostream>

#include "logger/logger.hpp"

namespace raphEngine
{
    std::unordered_map<std::type_index, std::shared_ptr<ISettingsCategory>>
        Settings::s_byType;
    std::unordered_map<std::string, ISettingsCategory*> Settings::s_byKey;
    std::vector<Settings::ChangeCallback> Settings::s_callbacks;

    bool Settings::Load(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            Logger::LogWarning("Settings not file found: ", path,
                               ", using defaults");
            return false;
        }

        try
        {
            nlohmann::json j;
            file >> j;

            for (auto& [key, category] : s_byKey)
            {
                if (j.contains(key))
                    category->FromJson(j.at(key));
            }

            Logger::LogDebug("Settings loaded: ", path);
            NotifyChanged();
            return true;
        }
        catch (const nlohmann::json::exception& e)
        {
            Logger::LogError("Settings parse error: ", e.what());
            return false;
        }
    }

    bool Settings::Save(const std::string& path)
    {
        try
        {
            nlohmann::json j;

            for (auto& [key, category] : s_byKey)
                j[key] = category->ToJson();

            std::ofstream file(path);
            if (!file.is_open())
            {
                Logger::LogError("Settings cannot write: ", path);
                return false;
            }

            file << j.dump(4);
            Logger::LogDebug("Settings saved: ", path);
            return true;
        }
        catch (const nlohmann::json::exception& e)
        {
            Logger::LogDebug("Settings write error: ", e.what());
            return false;
        }
    }

    void Settings::Reset()
    {
        for (auto& [key, category] : s_byKey)
            category->Reset();
        NotifyChanged();
    }

    void Settings::OnChanged(ChangeCallback cb)
    {
        s_callbacks.push_back(std::move(cb));
    }

    void Settings::NotifyChanged()
    {
        for (auto& cb : s_callbacks)
            cb();
    }
} // namespace raphEngine
