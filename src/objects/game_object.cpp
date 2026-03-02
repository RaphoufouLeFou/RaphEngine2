#include "RaphEngine2/objects/game_object.hpp"

#include <iostream>

namespace raphEngine::objects
{
    void GameObject::greed()
    {
        std::cout << "Hello, my name is \"" << name_ << "\"\n";
    }

    GameObject::GameObject()
    {
        name_ = "New GameObject";
    }

    void GameObject::render()
    {
        std::cout << "Rendering...\n";
    }
}