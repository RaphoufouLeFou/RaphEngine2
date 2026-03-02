#include "objects/game_object.hpp"

#include <iostream>

namespace raphEngine::objects
{
    void GameObject::greed()
    {
        std::cout << "Hello, my name is \"" << name_ << "\"\n";
    }

    GameObject::GameObject(std::string name) : Renderable()
    {
        name_ = name;
        transform_ = Transform();
    }

    GameObject::GameObject() : Renderable()
    {
        name_ = "New GameObject";
        transform_ = Transform();
    }

    GameObject::GameObject(GameObject& other) : Renderable()
    {
        name_ = other.name_;
        transform_ = other.transform_;
    }

    void GameObject::render()
    {
        std::cout << "Rendering...\n";
    }
}
