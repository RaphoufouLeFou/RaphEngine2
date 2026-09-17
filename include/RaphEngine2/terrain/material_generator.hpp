#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/terrain/chunk.hpp>
#include <RaphEngine2/terrain/noise.hpp>

#include <cstdint>
#include <functional>
#include <vector>

#include <glm/glm.hpp>

namespace raphEngine::terrain
{
    class RAPHENGINE_API MaterialGenerator
    {
    public:
        using HeightQueryFn = std::function<float(glm::vec2 worldXY)>;

        MaterialGenerator(uint32_t patchSeed, glm::vec2 worldHeightRange);

        std::vector<MaterialWeights>
        ComputeChunkWeights(const std::vector<float>& heightsMeters,
                            uint32_t resolution, glm::vec2 worldOrigin) const;

        MaterialWeights ComputeWeightsAt(glm::vec2 worldXY,
                                         const HeightQueryFn& heightAt) const;

    private:
        PerlinNoise patchNoise_;
        glm::vec2 worldHeightRange_;
    };
} // namespace raphEngine::terrain
