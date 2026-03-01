#include "RaphEngine2/game_object.hpp"

#include <iostream>

namespace raphEngine
{
    void GameObject::greed()
    {
        std::cout << "Hello, my name is \"" << name_ << "\"\n";
    }

    GameObject::GameObject()
    {
        name_ = "New GameObject";
    }

    void GameObject::render() const
    {
        std::cout << "Rendering...\n";
    }
}