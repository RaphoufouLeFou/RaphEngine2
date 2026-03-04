#include "objects/game_object.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace raphEngine::objects
{
    void GameObject::greed()
    {
        std::cout << "Hello, my name is \"" << name_ << "\"\n";
    }

    GameObject::GameObject(const std::string& name)
    {
        name_ = name;
        transform_ = Transform();
    }

    GameObject::GameObject(GameObject& other)
    {
        name_ = other.name_;
        transform_ = other.transform_;
    }

    component::Component* GameObject::get_component(size_t index)
    {
        return components_.at(index).get();
    }

    component::Component* GameObject::get_component(const std::string& name)
    {
        for (const auto& c : components_) {
            if(c.get()->get_name() == name)
            {
                return c.get();
            }
        }
        return nullptr;
    }

    void GameObject::remove_component(size_t index)
    {
        if (index >= components_.size())
            throw std::range_error(std::string("can't remove component at index ") + std::to_string(index));
        auto it = components_.begin();
        it+=index;
        components_.erase(it);
    }

    void GameObject::remove_component(const std::string& name)
    {
        for (auto it = components_.begin(); it != components_.end(); it++) {
            if((*it).get()->get_name() == name)
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

    GameObject& GameObject::find(const std::string& name)
    {

    }
}
