#pragma once

#include <iostream>
#include <string>

#include "export.hpp"

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
    };
} // namespace raphEngine::component
