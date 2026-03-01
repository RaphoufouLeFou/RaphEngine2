#pragma once

#include <string>

#include "renderable.hpp"
#include "transform.hpp"
#include "export.hpp"

namespace raphEngine
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

    virtual void render() const override;

private:
    std::string name_;
    Transform transform_;
};

}