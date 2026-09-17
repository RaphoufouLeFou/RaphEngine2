#pragma once

#include <RaphEngine2/export.hpp>

#include <cstdint>
#include <vector>

namespace raphEngine::terrain
{
    struct HydraulicErosionParams
    {
        uint32_t seed = 4321;
        uint32_t numDroplets = 400000;
        uint32_t maxDropletLifetime = 64;
        float inertia = 0.05f;
        float initialWaterVolume = 1.0f;
        float initialSpeed = 1.0f;
        float sedimentCapacityFactor = 5.0f;
        float minSedimentCapacity = 0.01f;
        float erodeSpeed = 0.5f;
        float depositSpeed = 0.15f;
        float evaporateSpeed = 0.005f;
        float gravity = 4.0f;
        int erosionRadius = 5;

        uint32_t workingResolution = 1024;

        static RAPHENGINE_API HydraulicErosionParams
        ForWorkingResolution(uint32_t workingResolution = 1024,
                             uint32_t seed = 4321, float intensity = 1.0f);
    };

    RAPHENGINE_API void
    ApplyHydraulicErosion(std::vector<float>& heights, uint32_t resolution,
                          const HydraulicErosionParams& params);
} // namespace raphEngine::terrain
