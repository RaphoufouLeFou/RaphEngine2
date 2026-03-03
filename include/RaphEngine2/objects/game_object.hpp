#pragma once

#include <memory>
#include <string>
#include <RaphEngine2/export.hpp>

#include "../renderable.hpp"
#include "transform.hpp"
#include "lod.hpp"

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
        std::unique_ptr<Lod> lods_; 
    };
}
