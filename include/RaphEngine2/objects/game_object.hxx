#pragma once

#include "game_object.hpp"
#include "RaphEngine2/component/component.hpp"
#include <RaphEngine2/logger/logger.hpp>

namespace raphEngine::objects
{

    template <Comp T, class... Args>
    void GameObject::add_component(Args&&... args)
    {
        std::unique_ptr<component::Component> c = std::make_unique<T>(args...);
        Logger::LogDebug("adding ", c->get_name());
        c->parent_object = this;
        components_.push_back(std::move(c));
    }

    template <class T>
    T* GameObject::get_first_component_of_type()
    {
        for (const auto& c : components_)
        {
            if (dynamic_cast<T*>(c.get()) != nullptr)
            {
                return dynamic_cast<T*>(c.get());
            }
        }
        return nullptr;
    }

    template <class T>
    std::vector<T*> GameObject::get_all_component_of_type()
    {
        std::vector<T*> res;
        for (const auto& c : components_)
        {
            if (dynamic_cast<T*>(c.get()) != nullptr)
            {
                res.push_back(c.get());
            }
        }
        return res;
    }

    template <class T>
    void GameObject::remove_all_component_of_type()
    {
        for (auto it = components_.begin(); it != components_.end(); it++)
        {
            if (dynamic_cast<T*>((*it).get()) != nullptr)
                it = components_.erase(it);
        }
    }

    template <class T>
    void GameObject::remove_first_component_of_type()
    {
        for (auto it = components_.begin(); it != components_.end(); it++)
        {
            if (dynamic_cast<T*>(*(*it).get()) != nullptr)
            {
                components_.erase(it);
                return;
            }
        }
    }
} // namespace raphEngine::objects
