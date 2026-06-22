#pragma once

#include <string>

#include "export.hpp"

namespace raphEngine
{
    class RAPHENGINE_API Time
    {
    public:
        static double GetTime();
        static double deltaTime;
    };
} // namespace raphEngine
