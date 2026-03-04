#pragma once

#include "export.hpp"
#include <string>

namespace raphEngine::component {
    class RAPHENGINE_API Component
    {
    public:
        virtual void Start() {}
        virtual void Update() {}

        virtual const std::string get_name() const = 0;
    };
}
