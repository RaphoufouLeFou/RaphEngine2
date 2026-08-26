#include "RaphEngine2/project_file/project_file.hpp"

#include <optional>
#include <nlohmann/json.hpp>
#include <RaphEngine2/logger/logger.hpp>

namespace raphEngine
{
    fs::path expand_home(const std::string& path)
    {
        if (!path.empty() && path[0] == '~')
        {
#ifdef _WIN32
            const char* home = std::getenv("USERPROFILE");
#else
            const char* home = std::getenv("HOME");
#endif
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
            Logger::LogFatal("No project file found at \"", path, "\"");
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
            Logger::LogFatal("Project file parse error: ", e.what());
            return std::nullopt;
        }
    }

    std::string Project::name = "Untitled project";
    std::string Project::setting_file_name = "settings.json";
    fs::path Project::path = get_default_projet();
    fs::path Project::main_scene_path = "assets/scenes/main.sc";

    bool Project::parse_project_file(const fs::path& path)
    {
        auto p = open_project_file(std::string(path));
        if (!p.has_value())
        {
            return false;
        }
        // Logger::LogInfo("Setting current path to ", path.);
        // std::filesystem::current_path(path.parent_path());
        auto j = p.value();

        Project::path = expand_home(path);
        if (j.contains("Name"))
            j.at("Name").get_to(name);
        if (j.contains("SettingFileName"))
            j.at("SettingFileName").get_to(setting_file_name);
        if (j.contains("MainSscenePath"))
            j.at("MainSscenePath").get_to(main_scene_path);

        return true;
    }

    void Project::create_default_project_file()
    {
        store_project_file();
    }

    fs::path Project::get_default_projet()
    {
        return expand_home(
            "~/RaphEngine_projects/Untitled_project/project.prj");
        ;
    }

    void Project::store_project_file()
    {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        if (!file.is_open())
        {
            Logger::LogFatal("Error writing at ", path);
            return;
        }

        nlohmann::json p = { { "Name", name },
                             { "SettingFileName", setting_file_name },
                             { "MainSscenePath", main_scene_path } };

        Logger::LogDebug("Saved at ", path);
        file << p.dump(4);
    }
} // namespace raphEngine
