#pragma once

#include <RaphEngine2/export.hpp>

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

namespace raphEngine::terrain
{

    class RAPHENGINE_API PerlinNoise
    {
    public:
        explicit PerlinNoise(uint32_t seed);

        float Sample(float x, float y) const noexcept;

    private:
        std::array<int, 512> permutation_{};
    };

    struct FractalNoiseParams
    {
        uint32_t seed = 1337;
        float baseFeatureScale = 512.0f;
        int octaves = 6;
        float persistence = 0.5f;
        float lacunarity = 2.0f;
    };

    RAPHENGINE_API float
    SampleFractalNoise(const PerlinNoise& noise, glm::vec2 worldXY,
                       const FractalNoiseParams& params) noexcept;
} // namespace raphEngine::terrain
