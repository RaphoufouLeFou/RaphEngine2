#include <RaphEngine2/scenes/scene.hpp>
#include <cstring>
#include <nlohmann/json.hpp>
#include <filesystem>
#include "graphics/skybox.hpp"
#include "imgui.h"
#include "logger/logger.hpp"
#include "objects/game_object.hpp"

namespace raphEngine
{

    bool Scene::remove_gameobject(objects::GameObject* obj)
    {
        if (destructing_)
            return false;

        size_t removed = std::erase_if(
            objects_, [obj](const std::unique_ptr<objects::GameObject>& ptr) {
                return ptr.get() == obj;
            });
        return removed > 0;
    }

    void Scene::add_gameobject(std::unique_ptr<objects::GameObject> obj)
    {
        if (!obj)
            return;
        objects_.push_back(std::move(obj));
    }

    const std::vector<std::unique_ptr<objects::GameObject>>&
    Scene::get_objects()
    {
        return objects_;
    }

    Scene::Scene(fs::path path)
    {
        // objects::Transform::root_childs.clear();

        if (path == "")
        {
            file_path_ = "default_scene.json";
            valid_ = parse_file(file_path_);
        }
        else
        {
            valid_ = parse_file(path);
        }
    }

    Scene::~Scene()
    {
        destructing_ = true;
        objects::Transform::root_childs.clear();
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
            Logger::LogError("No scene file found at ", path);
            return false;
        }

        nlohmann::json j;
        try
        {
            file >> j;

            if (j.contains("SkyBox"))
            {
                j.at("SkyBox").get_to(skybox_path_);

                graphics::Skybox::getInstance()->set_hdr(skybox_path_);
            }
            if (j.contains("Objects"))
            {
                for (const auto& objJson : j.at("Objects"))
                {
                    auto obj = reflection::Factory<objects::GameObject>::create(
                        objJson.at("__object_type").get<std::string>());
                    obj->fromJson(objJson);
                    objects_.push_back(std::move(obj));
                }
            }
        }
        catch (const std::exception& e)
        {
            Logger::LogError("Scene parse error: ", e.what());
            return false;
        }

        return true;
    }

    bool Scene::save_to_file(const fs::path& path)
    {
        Logger::LogDebug("Saving scene to ", path);
        fs::path p = file_path_;
        if (path != "")
            p = path;

        if (p == "")
        {
            Logger::LogError("Error, saving to empty path");
            return false;
        }

        std::ofstream file(p);
        if (!file.is_open())
        {
            Logger::LogError("Error writing at ", p);
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

        file_path_ = path_buffer;
        skybox_path_ = skybox_buffer;
    }

} // namespace raphEngine
