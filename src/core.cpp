#include "RaphEngine2/core.hpp"

#include <iostream>

#include "RaphEngine2/time_utils.hpp"
#include "graphics/ogl/opengl.hpp"
#include "objects/game_object.hpp"
#include "settings/graphics.hpp"
#include "settings/save_settings.hpp"

namespace raphEngine
{

    graphics::ogl::OpenGL renderer{};

    void Core::Init(const std::string& title)
    {
        (void)title;
        std::cout << "Hello world from RaphEngine2!" << std::endl;

        /*
        std::vector<std::unique_ptr<settings::SavableSetting>> sett =
            settings::SettingsSaver::load_settings("test.json");

        std::vector<settings::SavableSetting*> test;

        for (auto& s : sett)
        {
            test.push_back(s.get());
            std::cout << "test\n";
        }

        settings::SettingsSaver::save_settings(test, "test.json");
*/
        // std::cout << "Heree" << std::endl;
        renderer.Init(settings::Graphics(), "test");
    }

    void Core::Run()
    {
        std::cout << "running now from RaphEngine2!" << std::endl;

        execute_starts();

        while (1)
        {
            double start = Time::GetTime();
            execute_updates();
            execute_components_updates();
            renderer.Render();
            bool still_alive = renderer.Refresh();

            if (!still_alive)
            {
                break;
            }
            Time::deltaTime = (Time::GetTime() - start) / 1000.0;
        }

        std::cout << "exiting now!" << std::endl;
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
