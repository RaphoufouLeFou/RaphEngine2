#include "project_file.hpp"

#include <optional>
#include <nlohmann/json.hpp>
#include <RaphEngine2/logger/logger.hpp>

#ifdef ENGINE_BUILD
using namespace raphEngine;

fs::path expand_home(const std::string& path)
{
    if (!path.empty() && path[0] == '~')
    {
#    ifdef _WIN32
        const char* home = std::getenv("USERPROFILE");
#    else
        const char* home = std::getenv("HOME");
#    endif
        if (home)
            return fs::path(home) / path.substr(2);
    }
    return fs::path(path);
}

std::optional<nlohmann::json> open_project_file(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        Logger::LogCritical("No project file found at \"", path, "\"");
        return std::nullopt;
    }

    try
    {
        nlohmann::json j;
        file >> j;
        return j;
    }
    catch (const nlohmann::json::exception& e)
    {
        Logger::LogCritical("Project file parse error: ", e.what());
        return std::nullopt;
    }
}

std::string Project::name = "Untitled project";
std::string Project::setting_file_name = "settings.json";
fs::path Project::path =
    expand_home("~/RaphEngine_projects/Untitled_project/project.json");

bool Project::parse_project_file(const std::string& path)
{
    auto p = open_project_file(std::string(path));
    if (!p.has_value())
    {
        return false;
    }
    auto j = p.value();

    Project::path = expand_home(path);
    if (j.contains("Name"))
        j.at("Name").get_to(name);
    if (j.contains("SettingFileName"))
        j.at("SettingFileName").get_to(setting_file_name);

    return true;
}

void Project::store_project_file()
{
    fs::create_directories(path.parent_path());
    std::ofstream file(path);
    if (!file.is_open())
    {
        Logger::LogCritical("Error writing at ", path);
        return;
    }

    nlohmann::json p = { { "Name", name },
                         { "SettingFileName", setting_file_name } };

    Logger::LogDebug("Saved at ", path);
    file << p.dump(4);
}
#endif
