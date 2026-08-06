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
        static void StartGlobalTimer();
        static long long StopGlobalTimerAndGet_uS();
    };
} // namespace raphEngine
