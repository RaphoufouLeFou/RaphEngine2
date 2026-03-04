#pragma once

#include <RaphEngine2/export.hpp>
#include <filesystem>
#include <string>
#include <vector>
#include "serializable.hpp"
#include <nlohmann/json.hpp>

namespace raphEngine::settings {

    class RAPHENGINE_API SavableSetting : public Serializable
    {
    public:
        virtual nlohmann::json serialize() const override = 0;
        virtual bool deserialize(const nlohmann::json& input) override = 0;
        virtual constexpr std::string get_settings_name() const = 0;
    };

    class RAPHENGINE_API SettingsSaver
    {
        static void save_settings(std::vector<SavableSetting*>& settings, std::filesystem::path path);
        static std::vector<std::unique_ptr<SavableSetting>> load_settings(std::filesystem::path path);
    };
}
