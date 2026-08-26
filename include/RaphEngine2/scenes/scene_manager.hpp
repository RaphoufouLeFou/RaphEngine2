#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/scenes/scene.hpp>

namespace raphEngine
{

    class RAPHENGINE_API SceneManager
    {
    public:
        static void init();
        static bool load_scene(fs::path path);
        static Scene* get_active_scene();

#ifdef EDITOR_BUILD
        static void Imgui_update();
#endif

    private:
        static std::unique_ptr<Scene> active_scene_;
    };
} // namespace raphEngine
