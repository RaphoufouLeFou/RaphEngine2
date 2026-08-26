#pragma once

#include <RaphEngine2/export.hpp>
#include <filesystem>
#include <memory>
#include <vector>
#include <RaphEngine2/objects/game_object.hpp>

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

        bool remove_gameobject(objects::GameObject*);
        void add_gameobject(std::unique_ptr<objects::GameObject>);
        const std::vector<std::unique_ptr<objects::GameObject>>& get_objects();

    private:
        bool parse_file(fs::path path);
        bool save_to_file(const fs::path& path = "");

#ifdef EDITOR_BUILD
        void Imgui_update();
#endif

        friend class SceneManager;
        std::vector<std::unique_ptr<objects::GameObject>> objects_;
        fs::path skybox_path_ = "";
        fs::path file_path_;

        bool valid_ = false;
        bool destructing_ = false;
    };
} // namespace raphEngine
