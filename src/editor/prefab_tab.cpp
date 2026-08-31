#include <RaphEngine2/editor/prefab_tab.hpp>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "imgui.h"
#include "logger/logger.hpp"
#include "misc/cpp/imgui_stdlib.h"
#include "objects/game_object.hpp"
#include "scenes/scene_manager.hpp"

namespace raphEngine::editor
{
    fs::path PrefabTab::prefab_folder = "assets/prefabs/";

    std::unordered_map<std::string, nlohmann::json> prefabs_json;
    std::vector<std::string> displayed_prefabs;

    void PrefabTab::Init()
    {
        load_prefabs_json();
    }

    void PrefabTab::Update()
    {
        ImGui::Begin("Prefabs");
        static std::string s = prefab_folder.string();
        ImGui::InputText("Prefabs path", &s);
        prefab_folder = s;
        ImGui::SeparatorText("Avaliable prefabs");
        display_json();
        ImGui::End();
    }

    void PrefabTab::SetPrefabsPath(const fs::path& paf)
    {
        prefab_folder = paf;
    }

    const fs::path& PrefabTab::GetPrefabsPath()
    {
        return prefab_folder;
    }

    void PrefabTab::load_prefabs_json()
    {
        prefabs_json.clear();
        displayed_prefabs.clear();
        for (const auto& entry : fs::directory_iterator(prefab_folder))
        {
            if (entry.is_directory())
                continue;

            Logger::LogDebug("Found prefab : ", entry.path());
            std::ifstream file(entry.path());
            if (!file.is_open())
            {
                Logger::LogError("Error opening prefab file at ", entry.path());
                return;
            }

            nlohmann::json j;
            try
            {
                file >> j;
            }
            catch (const std::exception& e)
            {
                Logger::LogError("Prefab parse error: ", e.what());
                return;
            }
            if (j.contains("name_"))
            {
                std::string name = j.at("name_").get<std::string>();
                prefabs_json[name] = j;
                displayed_prefabs.push_back(name);
            }
            else
            {
                Logger::LogError("Invalid prefab format (failed to get name)");
            }
        }
    }

    void PrefabTab::display_json()
    {
        for (auto name : displayed_prefabs)
        {
            if (ImGui::Button(name.c_str()))
            {
                Logger::LogDebug("creating ", name);
                load_prefab(name);
            }
        }
    }

    void PrefabTab::load_prefab(const std::string& name)
    {
        if (!prefabs_json.contains(name))
        {
            Logger::LogError("Imposible to load prefab of ", name,
                             ": not found");
            return;
        }
        nlohmann::json j = prefabs_json[name];
        auto type_name = j.at("__object_type").get<std::string>();
        auto obj = reflection::Factory<objects::GameObject>::create(type_name);
        obj->fromJson(j);

        if (j.contains("parent_uuid"))
        {
            std::string parent_uuid;
            j.at("parent_uuid").get_to(parent_uuid);
            objects::GameObject* parent =
                objects::GameObject::find_uuid(parent_uuid);
            if (parent)
            {
                obj->get_transform().set_parent(&parent->get_transform(),
                                                false);
            }
        }

        if (SceneManager::get_active_scene())
            SceneManager::get_active_scene()->add_gameobject(std::move(obj));
    }

    void PrefabTab::save_as_prefab(const objects::GameObject* object)
    {
        nlohmann::json j = object->toJson();
        j["__object_type"] =
            reflection::Factory<objects::GameObject>::nameOf(*object);

        const auto& name = object->get_name();
        fs::path p = prefab_folder / (name + ".prefab");

        Logger::LogDebug("Saving prefab ", name, " to ", prefab_folder);

        std::ofstream file(p);
        if (!file.is_open())
        {
            Logger::LogError("Error writing at ", p);
            return;
        }
        file << j.dump(4);

        file.close();

        load_prefabs_json();
    }

} // namespace raphEngine::editor
