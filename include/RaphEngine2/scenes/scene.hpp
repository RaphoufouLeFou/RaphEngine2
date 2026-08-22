#pragma once

#include <RaphEngine2/export.hpp>
#include <filesystem>
#include <vector>
#include "objects/game_object.hpp"

namespace raphEngine
{
    class SceneManager;
    namespace fs = std::filesystem;

    class RAPHENGINE_API Scene
    {
    public:
        Scene(fs::path path);
        ~Scene();

        bool is_valid();

    private:
        bool parse_file(fs::path path);
        bool save_to_file(const fs::path& path = "");

#ifdef EDITOR_BUILD
        void Imgui_update();
#endif

        friend class SceneManager;
        std::vector<objects::GameObject*> objects_;
        fs::path skybox_path_ = "";
        fs::path file_path_;

        bool valid_ = false;
    };
} // namespace raphEngine
