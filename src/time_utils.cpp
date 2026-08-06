#include "time_utils.hpp"

#include <chrono>

namespace raphEngine
{
    double Time::deltaTime = 0;

    auto start = std::chrono::high_resolution_clock::now();

    double Time::GetTime()
    {
        auto now = std::chrono::high_resolution_clock::now();
        long long microseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(now - start)
                .count();
        return (double)microseconds / 1000;
    }
    static auto t = std::chrono::high_resolution_clock::now();
    void Time::StartGlobalTimer()
    {
        t = std::chrono::high_resolution_clock::now();
    }

    long long Time::StopGlobalTimerAndGet_uS()
    {
        auto now = std::chrono::high_resolution_clock::now();
        long long microseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(now - t)
                .count();
        return microseconds;
    }

} // namespace raphEngine
