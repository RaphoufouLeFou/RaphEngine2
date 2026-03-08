#include "RaphEngine2/core.hpp"

#include <iostream>

#include "objects/game_object.hpp"
#include "settings/save_settings.hpp"

namespace raphEngine
{
    void Core::Init(const std::string& title)
    {
        (void)title;
        std::cout << "Hello world from RaphEngine2!" << std::endl;

        std::vector<std::unique_ptr<settings::SavableSetting>> sett = settings::SettingsSaver::load_settings("test.json");
        std::vector<settings::SavableSetting*> test;
        for (auto &s : sett) {
            test.push_back(s.get());
            std::cout << "test\n";
        }
        
        settings::SettingsSaver::save_settings(test, "text.json");
        Run();
    }

    void Core::Run()
    {
        std::cout << "running now from RaphEngine2!" << std::endl;

        execute_starts();


        while (1)
        {
            execute_updates();
            execute_components_updates();
        }
    }

    void Core::execute_starts()
    {
        for (auto& go : objects::GameObject::spawned_game_objects_)
        {
            go.lock()->Awake();
        }

        for (auto& go : objects::GameObject::spawned_game_objects_)
        {
            go.lock()->Start();
        }
    }

    void Core::execute_updates()
    {
        for (auto& go : objects::GameObject::spawned_game_objects_)
        {
            go.lock()->Update();
        }
    }

    void Core::execute_components_updates()
    {
        for (auto& go : objects::GameObject::spawned_game_objects_)
        {
            go.lock()->update_components();
        }
    }
} // namespace raphEngine
