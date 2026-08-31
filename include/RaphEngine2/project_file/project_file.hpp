#pragma once

#include <string>
#include <filesystem>
#include <RaphEngine2/RaphEngine2.hpp>

namespace fs = std::filesystem;

namespace raphEngine
{
    class RAPHENGINE_API Project
    {
    public:
        bool static parse_project_file(const fs::path& path);
        void static store_project_file();
        void static create_default_project_file();
        fs::path static get_default_projet();

        static std::string name;
        static std::string setting_file_name;
        static fs::path path;
        static fs::path main_scene_path;
        static fs::path startup_screen_path;
    };
} // namespace raphEngine
