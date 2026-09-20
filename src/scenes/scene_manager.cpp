#include <RaphEngine2/scenes/scene_manager.hpp>
#include <RaphEngine2/scenes/scene.hpp>
#include <filesystem>
#include <memory>
#include "imgui.h"
#include "logger/logger.hpp"
#include "scenes/scene.hpp"

namespace raphEngine
{
    std::unique_ptr<Scene> SceneManager::active_scene_;

    void SceneManager::init()
    {
        load_scene("");
    }

    fs::path new_scene_path = "";
    bool new_scene_requested = false;

    void SceneManager::free_scene_internal()
    {
        active_scene_ = nullptr;
    }

    bool SceneManager::load_scene_internal()
    {
        if (!new_scene_requested)
        {
            return true;
        }

        new_scene_requested = false;
        Logger::LogDebug("Destroying old scene");
        active_scene_ = nullptr;
        Logger::LogDebug("Destroyed old scene");
        active_scene_ = std::make_unique<Scene>(new_scene_path);
        Logger::LogDebug("Loaded new scene");
        if (!active_scene_)
            return false;

        return active_scene_->is_valid();
    }

    void SceneManager::load_scene(const fs::path& path)
    {
        new_scene_requested = true;
        new_scene_path = path;
    }

    Scene* SceneManager::get_active_scene()
    {
        return active_scene_.get();
    }

    void SceneManager::Imgui_update()
    {
        static constexpr short buffer_size = 128;
        static char path_buffer[buffer_size] = "default_scene.json";
        ImGui::Begin("Scene manager");
        ImGui::InputText("Scene path", path_buffer, buffer_size);
        if (ImGui::Button("Load scene"))
        {
            load_scene(fs::path{ path_buffer });
        }
        if (ImGui::Button("Save scene"))
        {
            get_active_scene()->save_to_file(fs::path{ path_buffer });
        }
        if (ImGui::Button("Delete scene"))
        {
            delete active_scene_.release();
            init();
        }
        if (active_scene_)
        {
            active_scene_->Imgui_update();
        }
        ImGui::End();
    }
} // namespace raphEngine
