#include "RaphEngine2/core.hpp"
#include <memory>

#include "RaphEngine2/time_utils.hpp"
#include "editor/editor.hpp"
#include "editor/layout.hpp"
#include "graphics/camera.hpp"
#include "graphics/ogl/opengl.hpp"
#include "graphics/debug.hpp"
#include "logger/logger.hpp"
#include "objects/game_object.hpp"
#include "objects/transform.hpp"
#include "project_file/project_file.hpp"
#include "scenes/scene_manager.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"

#include "imgui.h"

namespace raphEngine
{
    graphics::ogl::OpenGL renderer{};
    static double current_fps_ = 60.0;
    bool Core::editor_mode_ = false;

    bool Core::is_editor_mode()
    {
        return editor_mode_;
    }

    graphics::GraphicApi* Core::getRenderer()
    {
        return &renderer;
    }

    double Core::GetFPS()
    {
        return current_fps_;
    }

    int Core::Launch(const std::string& project_file, bool editor_mode)
    {
        Logger::LogWarning("[DLL] sizeof(GameObject) = ",
                           sizeof(objects::GameObject));
        editor_mode_ = editor_mode;
        if (!Project::parse_project_file(project_file))
        {
            return 1;
        }

        Core::Init(Project::name);
        SceneManager::load_scene(Project::main_scene_path);

        if (Core::is_editor_mode())
        {
            Editor::Init();
        }

        Core::Run();

        Project::store_project_file();
        return 0;
    }

    void Core::Init(const std::string& title)
    {
        Logger::ConfigureLogger("log.txt", Logger::DEBUG);
        Logger::LogDebug("Hello world from RaphEngine2!");

        Settings::Register<GraphicsSettings>();
        Settings::Load("settings.json");

        if (Core::is_editor_mode())
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            ImGui::StyleColorsDark();
        }

        renderer.Init(title);
        graphics::Debug::getInstance()->Init();
        SceneManager::init();
    }

    void Core::Run()
    {
        Logger::LogDebug("running now from RaphEngine2!");

        static auto t = Time::GetTime();
        static double fps_avr = 0;
        static int avr_count = 0;

        while (1)
        {
            double start = Time::GetTime();
            renderer.StartFrame();

            if (Core::is_editor_mode())
            {
                Editor::Update();
            }

            fps_avr += 1.0f / Time::deltaTime;
            avr_count++;
            if (Time::GetTime() - t > 1000)
            {
                current_fps_ = fps_avr / avr_count;
                fps_avr = 0;
                avr_count = 0;
                t = Time::GetTime();
            }

            execute_updates();
            execute_components_updates();

            renderer.GetRmlUiRenderer().Update();
            renderer.Render();

            if (Core::is_editor_mode())
            {
                ImGui::Render();
            }
            bool still_alive = renderer.Refresh();

            if (!still_alive)
            {
                break;
            }
            Time::deltaTime = (Time::GetTime() - start) / 1000.0;
        }

        Settings::Save("settings.json");
        Logger::LogDebug("exiting now!");

        if (Core::is_editor_mode())
        {
            ImGui::DestroyContext();
        }
    }

    void Core::execute_updates()
    {
        if (Camera::get_active_camera())
        {
            Camera::get_active_camera()->CamUpdate();
        }

        if (Core::is_editor_mode())
        {
            editor::Layout::Update();
            ImGui::Begin("Inspector");
        }

        if (SceneManager::get_active_scene())
        {
            for (auto& go : SceneManager::get_active_scene()->get_objects())
            {
                if (!Core::is_editor_mode() && go->is_active)
                    go->pre_update();

                if (Core::is_editor_mode())
                {
                    go->ImGui_update();
                }

                if (!Core::is_editor_mode() && go->is_active)
                    go->Update();
            }
            if (Core::is_editor_mode())
            {
                ImGui::End();
            }
        }
    }

    void Core::execute_components_updates()
    {
        if (SceneManager::get_active_scene())
        {
            for (auto& go : SceneManager::get_active_scene()->get_objects())
            {
                if (go->is_active)
                    go->update_components();
            }
        }
    }

    void Core::Quit()
    {
        renderer.RequestQuit();
    }
} // namespace raphEngine
