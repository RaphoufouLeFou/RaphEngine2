#pragma once

#include <string>
#include <filesystem>
#include <RaphEngine2/RaphEngine2.hpp>

#ifdef EDITOR_BUILD
namespace fs = std::filesystem;

class Project
{
public:
    bool static parse_project_file(const std::string& path);
    void static store_project_file();

    static std::string name;
    static std::string setting_file_name;
    static fs::path path;
};
#endif
