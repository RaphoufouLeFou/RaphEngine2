#include <RaphEngine2/scenes/scene_manager.hpp>
#include <RaphEngine2/scenes/scene.hpp>
#include <memory>
#include "imgui.h"
#include "scenes/scene.hpp"

namespace raphEngine
{
    std::unique_ptr<Scene> SceneManager::active_scene_;

    bool SceneManager::load_scene(fs::path path)
    {
        active_scene_ = std::make_unique<Scene>(path);
        if (!active_scene_)
            return false;

        return active_scene_->is_valid();
    }

    Scene* SceneManager::get_active_scene()
    {
        return active_scene_.get();
    }

#ifdef EDITOR_BUILD
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
        }
        if (active_scene_)
        {
            active_scene_->Imgui_update();
        }
        ImGui::End();
    }
#endif

} // namespace raphEngine
