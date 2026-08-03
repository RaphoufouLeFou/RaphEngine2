#include "RaphEngine2/core.hpp"

#include "RaphEngine2/time_utils.hpp"
#include "graphics/ogl/opengl.hpp"
#include "logger/logger.hpp"
#include "objects/game_object.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"

#include "imgui.h"

namespace raphEngine
{
    graphics::ogl::OpenGL renderer{};
    

    void Core::Init(const std::string& title)
    {
        Logger::ConfigureLogger("log.txt", Logger::DEBUG);
        Logger::LogDebug("Hello world from RaphEngine2!");

        Settings::Register<GraphicsSettings>();
        Settings::Load("settings.json");

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |=
            ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
        io.ConfigFlags |=
            ImGuiConfigFlags_NoMouseCursorChange;
        io.ConfigFlags |=
            ImGuiConfigFlags_DockingEnable; // IF using Docking Branch
        ImGui::StyleColorsDark();

        renderer.Init(title);


    }

    void Core::Run()
    {
        Logger::LogDebug("running now from RaphEngine2!");

        execute_starts();

        while (1)
        {


            renderer.StartFrame();
            ImGui::NewFrame();

            ImGui::DockSpaceOverViewport(
                0, ImGui::GetMainViewport(),
                ImGuiDockNodeFlags_PassthruCentralNode);

            double start = Time::GetTime();
            execute_updates();
            execute_components_updates();

            renderer.Render();

            ImGui::Render();

            bool still_alive = renderer.Refresh();

            if (!still_alive)
            {
                break;
            }
            Time::deltaTime = (Time::GetTime() - start) / 1000.0;
        }

        Settings::Save("settings.json");
        Logger::LogDebug("exiting now!");


        ImGui::DestroyContext();
    }

    void Core::execute_starts()
    {
        for (auto& go : objects::GameObject::spawned_game_objects_)
        {
            go->Awake();
        }

        for (auto& go : objects::GameObject::spawned_game_objects_)
        {
            go->Start();
            go->start_components();
        }
    }

    void Core::execute_updates()
    {
        for (auto& go : objects::GameObject::spawned_game_objects_)
        {
            go->Update();
        }
    }

    void Core::execute_components_updates()
    {
        for (auto& go : objects::GameObject::spawned_game_objects_)
        {
            go->update_components();
        }
    }
} // namespace raphEngine
