#pragma once

#include <RaphEngine2/export.hpp>
#include <filesystem>
#include <string>
#include <vector>
#include "serializable.hpp"

namespace raphEngine::settings {

    class RAPHENGINE_API SavableSetting : public Serializable
    {
        virtual std::string serialize() override = 0;
        virtual bool deserialize(const std::string& input) override = 0;
        virtual constexpr std::string get_settings_name() const = 0;
    };

    class RAPHENGINE_API SettingsSaver
    {
        static void save_settings(std::vector<SavableSetting>, std::filesystem::path path);
        static std::vector<SavableSetting> load_settings(std::filesystem::path path);
    };
}
