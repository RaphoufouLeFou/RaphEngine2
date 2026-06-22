#pragma once

#include <iostream>
#include <string>

#include "RaphEngine2/export.hpp"

namespace raphEngine::objects
{
    class GameObject;
}

namespace raphEngine::component
{
    class RAPHENGINE_API Component
    {
    public:
        virtual ~Component() = default;

        virtual void Start()
        {}
        virtual void Update()
        {
            std::cout << "here from default component sadly\n";
        }

        virtual const std::string get_name() const
        {
            return "Empty component";
        };

        objects::GameObject* parent_object;
    };
} // namespace raphEngine::component
