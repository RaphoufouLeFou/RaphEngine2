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
        // objects::Transform::root_childs.clear();
    }

    bool Scene::is_valid()
    {
        return valid_;
    }

    objects::GameObject* Scene::find_uuid(const std::string& uuid)
    {
        for (const auto& go : get_objects())
        {
            if (Utils::compare_uuid(go->get_uuid(), uuid))
                return go.get();
        }
        return nullptr;
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
        }
        catch (const std::exception& e)
        {
            Logger::LogError("Scene parse error: ", e.what());
            return false;
        }

        try
        {
            if (j.contains("SkyBox"))
            {
                j.at("SkyBox").get_to(skybox_path_);

                graphics::Skybox::getInstance()->set_hdr(skybox_path_);
            }
        }
        catch (const std::exception& e)
        {
            Logger::LogError("SkyBox parse error: ", e.what());
        }
        if (j.contains("Objects"))
        {
            for (const auto& objJson : j.at("Objects"))
            {
                try
                {
                    auto type_name =
                        objJson.at("__object_type").get<std::string>();
                    auto obj = reflection::Factory<objects::GameObject>::create(
                        type_name);
                    if (!obj)
                    {
                        Logger::LogError("Could not find object of type ",
                                         type_name);
                        continue;
                    }
                    obj->fromJson(objJson);

                    if (objJson.contains("parent_uuid"))
                    {
                        std::string parent_uuid;
                        objJson.at("parent_uuid").get_to(parent_uuid);
                        objects::GameObject* parent = find_uuid(parent_uuid);
                        if (parent)
                        {
                            obj->get_transform().set_parent(
                                &parent->get_transform(), false);
                        }
                    }

                    objects_.push_back(std::move(obj));
                }
                catch (const std::exception& e)
                {
                    Logger::LogFatal("Object parse error: ", e.what());
                }
            }
        }

        return true;
    }

    void add_to_json(nlohmann::json& objectsJson, objects::GameObject* obj)
    {
        auto objJson = obj->toJson();
        objJson["__object_type"] =
            reflection::Factory<objects::GameObject>::nameOf(*obj);
        objectsJson.push_back(objJson);

        for (const auto& c : obj->get_transform().get_children())
        {
            add_to_json(objectsJson, c->parent_object);
        }
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
        for (const auto& obj : objects::Transform::root_childs)
        {
            add_to_json(objectsJson, obj->parent_object);
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
