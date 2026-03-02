#pragma once

#include <string>
#include <vector>
#include <RaphEngine2/export.hpp>

#include "../renderable.hpp"
#include "transform.hpp"
#include "object_mesh.hpp"

namespace raphEngine::objects
{
    class RAPHENGINE_API GameObject : private Renderable
    {
    public:
        void greed();
        GameObject();
        GameObject(std::string name);
        GameObject(GameObject& other);
        ~GameObject() = default;

        virtual void Start() {}
        virtual void Update() {}

    private:
        void render();
        std::string name_;
        Transform transform_;
    };
}
