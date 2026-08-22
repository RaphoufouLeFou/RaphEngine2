#include <RaphEngine2/scenes/scene.hpp>
#include <cstring>
#include <nlohmann/json.hpp>
#include <filesystem>
#include "imgui.h"
#include "objects/game_object.hpp"

namespace raphEngine
{

    Scene::Scene(fs::path path)
    {
        objects::Transform::root_childs.clear();
        for (auto obj : objects::GameObject::spawned_game_objects_)
        {
            objects::GameObject::destroy(*obj);
        }
        objects::GameObject::spawned_game_objects_.clear();

        if (path == "")
        {
            file_path_ = "default_scene.json";
            save_to_file(file_path_);
            valid_ = parse_file(file_path_);
        }
        else
        {
            valid_ = parse_file(path);
        }
    }

    Scene::~Scene()
    {
        objects::Transform::root_childs.clear();
        for (auto obj : objects_)
        {
            objects::GameObject::destroy(*obj);
        }
    }

    bool Scene::is_valid()
    {
        return valid_;
    }

    bool Scene::parse_file(fs::path path)
    {
        this->file_path_ = path;
        std::ifstream file(path);
        if (!file.is_open())
        {
            Logger::LogCritical("No scene file found at ", path);
            return false;
        }

        nlohmann::json j;
        try
        {
            file >> j;

            if (j.contains("SkyBox"))
                j.at("SkyBox").get_to(skybox_path_);

            if (j.contains("Objects"))
            {
                for (const auto& objJson : j.at("Objects"))
                {
                    auto obj = reflection::Factory<objects::GameObject>::create(
                        objJson.at("__object_type").get<std::string>());
                    obj->fromJson(objJson);
                    objects_.push_back(obj.release());
                }
            }
        }
        catch (const std::exception& e)
        {
            Logger::LogCritical("Scene parse error: ", e.what());
            return false;
        }

        return true;
    }

    bool Scene::save_to_file(const fs::path& path)
    {
        fs::path p = file_path_;
        if (path != "")
            p = path;

        if (p == "")
        {
            Logger::LogCritical("Error, saving to empty path");
            return false;
        }

        std::ofstream file(p);
        if (!file.is_open())
        {
            Logger::LogCritical("Error writing at ", p);
            return false;
        }

        nlohmann::json objectsJson = nlohmann::json::array();
        for (const auto& obj : objects_)
        {
            auto objJson = obj->toJson();
            objJson["__object_type"] =
                reflection::Factory<objects::GameObject>::nameOf(*obj);
            objectsJson.push_back(objJson);
        }

        nlohmann::json sc = {
            { "SkyBox", skybox_path_ },
            { "Objects", objectsJson },
        };
        file << sc.dump(4);
        Logger::LogDebug("Saved scene at ", p);
        return true;
    }

#ifdef EDITOR_BUILD
    void Scene::Imgui_update()
    {
        static constexpr short buffer_size = 128;
        char path_buffer[buffer_size] = { 0 };
        char skybox_buffer[buffer_size] = { 0 };
        std::memcpy(path_buffer, file_path_.string().c_str(),
                    file_path_.string().size());

        std::memcpy(skybox_buffer, skybox_path_.string().c_str(),
                    skybox_path_.string().size());

        ImGui::Separator();
        ImGui::Text("Scene");
        ImGui::InputText("path", path_buffer, buffer_size);
        ImGui::InputText("skybox", skybox_buffer, buffer_size);

        int count = objects_.size();
        ImGui::InputInt("Object count", &count);

        int total_count = objects::GameObject::spawned_game_objects_.size();
        ImGui::InputInt("Total objects count", &total_count);

        file_path_ = path_buffer;
        skybox_path_ = skybox_buffer;
    }
#endif

} // namespace raphEngine
