#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/scenes/scene.hpp>
#include <RaphEngine2/core.hpp>

namespace raphEngine
{

    class RAPHENGINE_API SceneManager
    {
    public:
        static void init();
        static void load_scene(const fs::path& path);
        static Scene* get_active_scene();

        static void Imgui_update();

    private:
        static std::unique_ptr<Scene> active_scene_;

        friend class Core;
        static bool load_scene_internal();
        static void free_scene_internal();
    };
} // namespace raphEngine
