#include "objects/game_object.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <algorithm>
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
        transform_.parent_object = this;
        spawned_game_objects_.push_back(this);
    }

    GameObject::GameObject()
    {
        id_ = spawned_game_objects_.size();
        name_ = "New GameObject " + std::to_string(id_);
        transform_.parent_object = this;
        spawned_game_objects_.push_back(this);
    }

    GameObject::GameObject(const GameObject& other)
    {
        name_ = other.name_;
        transform_ = other.transform_;
        transform_.parent_object = this;
        transform_.set_parent(other.transform_.get_parent());
        spawned_game_objects_.push_back(this);
    }

    std::shared_ptr<GameObject> GameObject::instanciate(const GameObject& from)
    {
        return std::make_shared<GameObject>(from);
    }

    void GameObject::destroy_internal()
    {
        auto position = std::find(spawned_game_objects_.begin(),
                                  spawned_game_objects_.end(), this);
        if (position != spawned_game_objects_.end())
            spawned_game_objects_.erase(position);
    }

    void GameObject::destroy(GameObject& object)
    {
        object.is_active = false;
        object.destroy_internal();
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
        if (transform_.get_children().size() == 0 || true)
        {
            ImGui::Bullet();
            if (ImGui::Selectable(name_.c_str(), &inspected)
                && selected == this)
            {
                selected = nullptr;
                inspected = false;
            }
            if (inspected)
            {
                selected = this;
            }
        }
        else
        {
            bool unfolded = ImGui::TreeNode(name_.c_str());

            if (inspected)
            {
                selected = this;
            }

            if (unfolded)
            {
                ImGui::TreePop();
            }
        }

        ImGui::Indent();
        for (auto* t : transform_.get_children())
        {
            t->parent_object->ImGui_layout();
        }
        ImGui::Unindent();
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
                                  1);
                ImGui::DragFloat3("Rotation", &get_transform().get_rotation().x,
                                  1);
                ImGui::DragFloat3("Scale", &get_transform().get_scale().x,
                                  0.1f);
                ImGui::Text("Model matrix");
                glm::mat4 matrix = get_transform().get_model_matrix();
                ImGui::InputFloat4("[0]", &matrix[0].x);
                ImGui::InputFloat4("[1]", &matrix[1].x);
                ImGui::InputFloat4("[2]", &matrix[2].x);
                ImGui::InputFloat4("[3]", &matrix[3].x);
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
        destroy_internal();
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
