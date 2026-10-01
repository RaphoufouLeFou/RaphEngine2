#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/material_generator.hpp>

#include <algorithm>
#include <cmath>

#include <tbb/parallel_for.h>

namespace raphEngine::terrain
{
    namespace
    {
        constexpr float kSlopeSampleTexels = 2.0f;
        constexpr float kRockPatchScale = 60.0f;
        constexpr float kJitterScale = 24.0f;
        constexpr float kJitterFraction = 0.05f;

        constexpr float kAlpineZoneLowFraction = 0.32f;
        constexpr float kAlpineZoneHighFraction = 0.62f;

        constexpr float kRockSlopeLow = 0.22f;
        constexpr float kRockSlopeHigh = 0.34f;

        constexpr float kDirtPatchBaseScale = 110.0f;
        constexpr int kDirtPatchOctaves = 4;
        constexpr float kDirtPatchPersistence = 0.5f;
        constexpr float kDirtPatchLacunarity = 2.75f;
    } // namespace

    MaterialGenerator::MaterialGenerator(uint32_t patchSeed,
                                         glm::vec2 worldHeightRange)
        : patchNoise_(patchSeed)
        , worldHeightRange_(worldHeightRange)
    {}

    MaterialWeights
    MaterialGenerator::ComputeWeightsAt(glm::vec2 worldXY,
                                        const HeightQueryFn& heightAt) const
    {
        const float heightSpan =
            std::max(worldHeightRange_.y - worldHeightRange_.x, 1.0f);

        const float h = heightAt(worldXY);
        const float relativeHeight =
            std::clamp((h - worldHeightRange_.x) / heightSpan, 0.0f, 1.0f);

        const float mL =
            heightAt(worldXY - glm::vec2(kSlopeSampleTexels, 0.0f));
        const float mR =
            heightAt(worldXY + glm::vec2(kSlopeSampleTexels, 0.0f));
        const float mD =
            heightAt(worldXY - glm::vec2(0.0f, kSlopeSampleTexels));
        const float mU =
            heightAt(worldXY + glm::vec2(0.0f, kSlopeSampleTexels));

        const glm::vec3 normal = glm::normalize(
            glm::vec3(mL - mR, mD - mU, 2.0f * kSlopeSampleTexels));
        const float slope = 1.0f - normal.z;

        auto patch = [&](glm::vec2 xy, float scale) -> float {
            return patchNoise_.Sample(xy.x / scale, xy.y / scale) * 0.5f + 0.5f;
        };

        auto dirtPatch = [&](glm::vec2 xy) -> float {
            FractalNoiseParams params;
            params.baseFeatureScale = kDirtPatchBaseScale;
            params.octaves = kDirtPatchOctaves;
            params.persistence = kDirtPatchPersistence;
            params.lacunarity = kDirtPatchLacunarity;
            return SampleFractalNoise(patchNoise_, xy, params) * 0.5f + 0.5f;
        };

        const float jitter =
            patch(worldXY + glm::vec2(5.2f, 9.8f), kJitterScale) - 0.5f;
        const float jitterHeight = jitter * kJitterFraction;

        const float alpineMask = glm::smoothstep(
            kAlpineZoneLowFraction + jitterHeight,
            kAlpineZoneHighFraction + jitterHeight, relativeHeight);

        const float rockBySlope =
            glm::smoothstep(kRockSlopeLow + jitter * 0.08f,
                            kRockSlopeHigh + jitter * 0.08f, slope);

        const float rockByNoise = glm::smoothstep(
            0.6f, 0.85f,
            patch(worldXY + glm::vec2(37.1f, 58.9f), kRockPatchScale));

        const float rockWeight = std::max(rockBySlope, rockByNoise);

        const float snowWeight = alpineMask * (1.0f - rockBySlope);

        const float dirtByNoise =
            glm::smoothstep(0.55f + jitter * 0.1f, 0.8f,
                            dirtPatch(worldXY + glm::vec2(91.7f, 12.3f)));
        const float dirtWeight =
            dirtByNoise * (1.0f - rockWeight) * (1.0f - snowWeight);

        MaterialWeights w{};
        w.rock = static_cast<uint8_t>(
            std::clamp(rockWeight, 0.0f, 1.0f) * 255.0f + 0.5f);
        w.snow = static_cast<uint8_t>(
            std::clamp(snowWeight, 0.0f, 1.0f) * 255.0f + 0.5f);
        w.dirt = static_cast<uint8_t>(
            std::clamp(dirtWeight, 0.0f, 1.0f) * 255.0f + 0.5f);
        return w;
    }

    std::vector<MaterialWeights> MaterialGenerator::ComputeChunkWeights(
        const std::vector<float>& heightsMeters, uint32_t resolution,
        glm::vec2 worldOrigin) const
    {
        std::vector<MaterialWeights> result(heightsMeters.size());

        auto heightAt = [&](glm::vec2 worldXY) -> float {
            const glm::vec2 local = worldXY - worldOrigin;
            const int x = std::clamp(static_cast<int>(std::lround(local.x)), 0,
                                     static_cast<int>(resolution) - 1);
            const int y = std::clamp(static_cast<int>(std::lround(local.y)), 0,
                                     static_cast<int>(resolution) - 1);
            return heightsMeters[static_cast<size_t>(y) * resolution + x];
        };

        tbb::parallel_for(uint32_t(0), resolution, [&](uint32_t ty) {
            for (uint32_t tx = 0; tx < resolution; ++tx)
            {
                const glm::vec2 worldXY = worldOrigin
                    + glm::vec2(static_cast<float>(tx), static_cast<float>(ty));
                result[ty * resolution + tx] =
                    ComputeWeightsAt(worldXY, heightAt);
            }
        });

        return result;
    }
} // namespace raphEngine::terrain
