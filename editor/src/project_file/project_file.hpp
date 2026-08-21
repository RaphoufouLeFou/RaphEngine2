#pragma once

#include <string>
#include <filesystem>

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
