#pragma once

#include "RaphEngine2/RaphEngine2.hpp"
#include "RaphEngine2/export.hpp"

namespace raphEngine::resources
{
    class RAPHENGINE_API Resource
    {
    public:
        Resource(const std::string& path);
        virtual void TODO () = 0;
    };
} // namespace raphEngine::resources
