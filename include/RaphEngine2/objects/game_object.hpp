#pragma once

#include <string>

#include "RaphEngine2/renderable.hpp"
#include "transform.hpp"
#include <RaphEngine2/export.hpp>

namespace raphEngine::objects
{

class RAPHENGINE_API GameObject : private Renderable
{
public:
    void greed();
    GameObject();
    GameObject(std::string name)
        : name_ {name}
    {}
    GameObject(GameObject& other) = default;
    ~GameObject() = default;

    virtual void render();

private:
    std::string name_;
    Transform transform_;
    
};

}