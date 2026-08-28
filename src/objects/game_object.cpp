#include "objects/game_object.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <RaphEngine2/logger/logger.hpp>
#include <unordered_map>
#include <utility>
#include <vector>
#include "component/component.hpp"
#include "component/mesh_component.hpp"
#include "imgui.h"

#include "misc/cpp/imgui_stdlib.h"
#include "objects/transform.hpp"
#include "scenes/reflection.hpp"
#include "scenes/scene.hpp"
#include "scenes/scene_manager.hpp"
#include "utils.hpp"

#include "editor/layout.hpp"

namespace raphEngine::objects
{

    void GameObject::greed()
    {
        Logger::LogInfo("Hello, my name is \"", name_, "\"");
    }

    GameObject::GameObject(const std::string& name)
    {
        id_ = Utils::get_id();
        uuid_ = Utils::get_uuid();
        name_ = name;
        transform_.parent_object = this;
    }

    GameObject::GameObject()
    {
        id_ = Utils::get_id();
        uuid_ = Utils::get_uuid();
        name_ = "New GameObject " + std::to_string(id_);
        transform_.parent_object = this;
    }

    GameObject::GameObject(const GameObject& other)
    {
        id_ = Utils::get_id();
        uuid_ = Utils::get_uuid();
        name_ = other.name_;
        transform_ = other.transform_;
        transform_.parent_object = this;
        transform_.set_parent(other.transform_.get_parent());
    }

    GameObject* GameObject::instanciate(const GameObject& from)
    {
        if (!SceneManager::get_active_scene())
        {
            return nullptr;
        }
        auto go = std::make_unique<GameObject>(from);
        auto res = go.get();
        SceneManager::get_active_scene()->add_gameobject(std::move(go));
        return res;
    }

    void GameObject::destroy_internal()
    {
        if (SceneManager::get_active_scene())
            SceneManager::get_active_scene()->remove_gameobject(this);
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

    void GameObject::ImGui_update()
    {
        auto c = get_first_component_of_type<component::MeshComponent>();
        if (editor::Layout::IsSelected(this))
        {
            if (c)
                c->outline_ = true;
            std::string display_name = name_ + "###" + std::to_string(id_);

            ImGui::Checkbox("Is active", &is_active);
            ImGui::InputText("Name", &name_);
            ImGui::InputInt("RayCast layer", &raycast_layer_);
            ImGui::Checkbox("Has started", &has_started);
            int id_const = id_;
            ImGui::InputInt("Id", &id_const);
            ImGui::LabelText("UUID", "%s", uuid_.c_str());

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Transform"))
            {
                ImGui::DragFloat3("Position", &get_transform().position_.x, 1);
                ImGui::DragFloat3("Rotation", &get_transform().rotation_.x, 1);
                ImGui::DragFloat3("Scale", &get_transform().scale_.x, 0.1f);

                get_transform().can_have_moved = true;
            }

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Components"))
            {
                for (size_t i = 0; i < components_.size(); i++)
                {
                    ImGui::PushID(i);
                    components_[i]->ImGuiPrint();
                    ImGui::PopID();
                    ImGui::Separator();
                }

                if (ImGui::Button("Add component"))
                {
                    ImGui::OpenPopup("add_component_popup");
                }

                if (ImGui::BeginPopup("add_component_popup"))
                {
                    auto aval_comps = reflection::Factory<
                        component::Component>::allRegistered();
                    ImGui::SeparatorText("Component avalable");
                    for (const auto& [names, creator] : aval_comps)
                    {
                        if (ImGui::Selectable(names.c_str()))
                        {
                            add_component(creator());
                        }
                    }

                    ImGui::EndPopup();
                }
            }
            // ImGui::End();
            // ImGui::TreePop();
        }
        else
        {
            if (c)
                c->outline_ = false;
        }
    }

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
        if (SceneManager::get_active_scene())
        {
            for (const auto& go :
                 SceneManager::get_active_scene()->get_objects())
            {
                if (go->name_ == name)
                    return go.get();
            }
        }
        return nullptr;
    }

    void GameObject::add_component(std::unique_ptr<component::Component> c)
    {
        Logger::LogDebug("adding ", c->get_name());
        c->parent_object = this;
        c->Start();
        components_.push_back(std::move(c));
    }

    nlohmann::json GameObject::toJson() const
    {
        nlohmann::json j = reflection::toJson(*this);
        j["transform_"] = transform_;
        j["components_"] = components_;
        return j;
    }

    void GameObject::fromJson(const nlohmann::json& j)
    {
        reflection::fromJson(*this, j);

        Logger::LogDebug("Parsing a gameobject");

        if (j.contains("transform_"))
        {
            const nlohmann::json& t = j.at("transform_");
            Logger::LogDebug("Adding transform");
            t.at("position").get_to(transform_.position_);
            t.at("rotation").get_to(transform_.rotation_);
            t.at("scale").get_to(transform_.scale_);
            transform_.can_have_moved = true;
        }

        if (j.contains("components_"))
        {
            const nlohmann::json& c = j.at("components_");
            Logger::LogDebug("Found ", c.size(), " Components");
            for (const auto& comp : c)
            {
                auto ptr = component::Component::parse_from_json(comp);

                Logger::LogDebug("Fast adding ", ptr->get_name());
                add_component(std::move(ptr));
            }
        }
    }

} // namespace raphEngine::objects
