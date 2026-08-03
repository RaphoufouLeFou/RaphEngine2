#include "objects/game_object.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <RaphEngine2/logger/logger.hpp>
#include "imgui.h"

#include "misc/cpp/imgui_stdlib.h"

namespace raphEngine::objects
{
    std::vector<GameObject*> GameObject::spawned_game_objects_;

    void GameObject::greed()
    {
        Logger::LogInfo("Hello, my name is \"", name_, "\"");
    }

    GameObject::GameObject(const std::string& name)
    {
        id_ = spawned_game_objects_.size();
        name_ = name;
        transform_ = Transform();
        spawned_game_objects_.push_back(this);
    }

    GameObject::GameObject()
    {
        id_ = spawned_game_objects_.size();
        name_ = "New GameObject " + std::to_string(id_);
        transform_ = Transform();
        spawned_game_objects_.push_back(this);
    }

    GameObject::GameObject(GameObject& other)
    {
        name_ = other.name_;
        transform_ = other.transform_;
        spawned_game_objects_.push_back(this);
    }

    void GameObject::pre_update()
    {
        if (!has_started)
        {
            Awake();
            Start();
            has_started = true;
        }
    }

#ifdef EDITOR_BUILD

    static GameObject* selected = nullptr;
    void GameObject::ImGui_layout()
    {
        inspected = selected == this;
        ImGui::Bullet();
        if (ImGui::Selectable(name_.c_str(), &inspected) && selected == this)
        {
            selected = nullptr;
            inspected = false;
        }
        if (inspected)
        {
            selected = this;
        }
    }

    void GameObject::ImGui_update()
    {
        if (inspected)
        {
            std::string display_name = name_ + "###" + std::to_string(id_);

            ImGui::Checkbox("Is active", &is_active);
            ImGui::InputText("Name", &name_);
            ImGui::InputInt("RayCast layer", &raycast_layer_);
            ImGui::Checkbox("Has started", &has_started);

            if (ImGui::CollapsingHeader("Transform"))
            {
                ImGui::DragFloat3("Position", &get_transform().get_position().x,
                                  0.05);
                ImGui::DragFloat3("Rotation", &get_transform().get_rotation().x,
                                  0.01);
                ImGui::DragFloat3("Scale", &get_transform().get_scale().x,
                                  0.01);
            }

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Components"))
            {
                for (auto& c : components_)
                {
                    c->ImGuiPrint();
                }
            }
            // ImGui::End();
            // ImGui::TreePop();
        }
    }

#endif

    GameObject::~GameObject()
    {
        auto position = std::find(spawned_game_objects_.begin(),
                                  spawned_game_objects_.end(), this);
        if (position != spawned_game_objects_.end())
            spawned_game_objects_.erase(position);
    }

    component::Component* GameObject::get_component(size_t index)
    {
        return components_.at(index).get();
    }

    component::Component* GameObject::get_component(const std::string& name)
    {
        for (const auto& c : components_)
        {
            if (c.get()->get_name() == name)
            {
                return c.get();
            }
        }
        return nullptr;
    }

    void GameObject::remove_component(size_t index)
    {
        if (index >= components_.size())
            throw std::range_error(
                std::string("can't remove component at index ")
                + std::to_string(index));
        auto it = components_.begin();
        it += index;
        components_.erase(it);
    }

    void GameObject::remove_component(const std::string& name)
    {
        for (auto it = components_.begin(); it != components_.end(); it++)
        {
            if ((*it).get()->get_name() == name)
                it = components_.erase(it);
        }
    }

    void GameObject::start_components()
    {
        for (auto& c : components_)
            c->Start();
    }

    void GameObject::update_components()
    {
        for (auto& c : components_)
            c->Update();
    }

    std::string& GameObject::get_name()
    {
        return name_;
    }

    objects::Transform& GameObject::get_transform()
    {
        return transform_;
    }

    GameObject* GameObject::find(const std::string& name)
    {
        for (auto& go : spawned_game_objects_)
        {
            if (go->name_ == name)
                return go;
        }
        return nullptr;
    }
} // namespace raphEngine::objects
