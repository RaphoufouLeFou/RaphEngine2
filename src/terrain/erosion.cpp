#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/erosion.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include <glm/glm.hpp>

namespace raphEngine::terrain
{
    namespace
    {
        struct HeightAndGradient
        {
            float height;
            float gradientX;
            float gradientY;
        };

        HeightAndGradient
        CalculateHeightAndGradient(const std::vector<float>& heights,
                                   uint32_t resolution, float posX, float posY)
        {
            const int coordX = static_cast<int>(posX);
            const int coordY = static_cast<int>(posY);

            const float x = posX - static_cast<float>(coordX);
            const float y = posY - static_cast<float>(coordY);

            const int nodeIndexNW =
                coordY * static_cast<int>(resolution) + coordX;
            const float heightNW = heights[nodeIndexNW];
            const float heightNE = heights[nodeIndexNW + 1];
            const float heightSW = heights[nodeIndexNW + resolution];
            const float heightSE = heights[nodeIndexNW + resolution + 1];

            const float gradientX =
                (heightNE - heightNW) * (1.0f - y) + (heightSE - heightSW) * y;
            const float gradientY =
                (heightSW - heightNW) * (1.0f - x) + (heightSE - heightNE) * x;

            const float height = heightNW * (1.0f - x) * (1.0f - y)
                + heightNE * x * (1.0f - y) + heightSW * (1.0f - x) * y
                + heightSE * x * y;

            return { height, gradientX, gradientY };
        }

        struct BrushPoint
        {
            int dx;
            int dy;
            float weight;
        };

        std::vector<BrushPoint> BuildErosionBrush(int radius)
        {
            std::vector<BrushPoint> brush;
            float weightSum = 0.0f;

            for (int dy = -radius; dy <= radius; ++dy)
            {
                for (int dx = -radius; dx <= radius; ++dx)
                {
                    const float sqrDist = static_cast<float>(dx * dx + dy * dy);
                    if (sqrDist < static_cast<float>(radius * radius))
                    {
                        const float dist = std::sqrt(sqrDist);
                        const float weight =
                            1.0f - dist / static_cast<float>(radius);
                        weightSum += weight;
                        brush.push_back({ dx, dy, weight });
                    }
                }
            }

            for (BrushPoint& b : brush)
            {
                b.weight /= weightSum;
            }

            return brush;
        }

        void SimulateDroplets(std::vector<float>& heights, uint32_t resolution,
                              const HydraulicErosionParams& params)
        {
            const std::vector<BrushPoint> brush =
                BuildErosionBrush(std::max(params.erosionRadius, 1));

            std::mt19937 rng(params.seed);
            std::uniform_real_distribution<float> spawnDist(
                0.0f, static_cast<float>(resolution) - 1.0001f);
            std::uniform_real_distribution<float> angleDist(0.0f,
                                                            6.28318530718f);

            const float maxCoord = static_cast<float>(resolution) - 1.0001f;

            for (uint32_t iter = 0; iter < params.numDroplets; ++iter)
            {
                float posX = spawnDist(rng);
                float posY = spawnDist(rng);
                float dirX = 0.0f;
                float dirY = 0.0f;
                float speed = params.initialSpeed;
                float water = params.initialWaterVolume;
                float sediment = 0.0f;

                for (uint32_t lifetime = 0;
                     lifetime < params.maxDropletLifetime; ++lifetime)
                {
                    const int nodeX = static_cast<int>(posX);
                    const int nodeY = static_cast<int>(posY);
                    const float cellOffsetX = posX - static_cast<float>(nodeX);
                    const float cellOffsetY = posY - static_cast<float>(nodeY);

                    const HeightAndGradient oldHag = CalculateHeightAndGradient(
                        heights, resolution, posX, posY);

                    dirX = dirX * params.inertia
                        - oldHag.gradientX * (1.0f - params.inertia);
                    dirY = dirY * params.inertia
                        - oldHag.gradientY * (1.0f - params.inertia);

                    const float dirLen = std::sqrt(dirX * dirX + dirY * dirY);
                    if (dirLen < 1e-8f)
                    {
                        const float angle = angleDist(rng);
                        dirX = std::cos(angle);
                        dirY = std::sin(angle);
                    }
                    else
                    {
                        dirX /= dirLen;
                        dirY /= dirLen;
                    }

                    const float newPosX = posX + dirX;
                    const float newPosY = posY + dirY;

                    if (newPosX < 0.0f || newPosX >= maxCoord || newPosY < 0.0f
                        || newPosY >= maxCoord)
                    {
                        break;
                    }

                    const float oldHeight = oldHag.height;
                    const HeightAndGradient newHag = CalculateHeightAndGradient(
                        heights, resolution, newPosX, newPosY);
                    const float deltaHeight = newHag.height - oldHeight;

                    const float sedimentCapacity =
                        std::max(-deltaHeight * speed * water
                                     * params.sedimentCapacityFactor,
                                 params.minSedimentCapacity);

                    if (sediment > sedimentCapacity || deltaHeight > 0.0f)
                    {
                        const float amountToDeposit = (deltaHeight > 0.0f)
                            ? std::min(deltaHeight, sediment)
                            : (sediment - sedimentCapacity)
                                * params.depositSpeed;
                        sediment -= amountToDeposit;

                        const int idx =
                            nodeY * static_cast<int>(resolution) + nodeX;
                        heights[idx] += amountToDeposit * (1.0f - cellOffsetX)
                            * (1.0f - cellOffsetY);
                        heights[idx + 1] += amountToDeposit * cellOffsetX
                            * (1.0f - cellOffsetY);
                        heights[idx + resolution] += amountToDeposit
                            * (1.0f - cellOffsetX) * cellOffsetY;
                        heights[idx + resolution + 1] +=
                            amountToDeposit * cellOffsetX * cellOffsetY;
                    }
                    else
                    {
                        const float amountToErode = std::min(
                            (sedimentCapacity - sediment) * params.erodeSpeed,
                            -deltaHeight);

                        for (const BrushPoint& b : brush)
                        {
                            const int ex = nodeX + b.dx;
                            const int ey = nodeY + b.dy;
                            if (ex < 0 || ex >= static_cast<int>(resolution)
                                || ey < 0 || ey >= static_cast<int>(resolution))
                            {
                                continue;
                            }

                            const int eidx =
                                ey * static_cast<int>(resolution) + ex;
                            const float weightedErode =
                                amountToErode * b.weight;
                            const float actualErode =
                                std::min(heights[eidx], weightedErode);
                            heights[eidx] -= actualErode;
                            sediment += actualErode;
                        }
                    }

                    speed = std::sqrt(std::max(
                        0.0f, speed * speed + deltaHeight * params.gravity));
                    water *= (1.0f - params.evaporateSpeed);

                    posX = newPosX;
                    posY = newPosY;

                    if (water < 1e-4f)
                    {
                        break;
                    }
                }
            }
        }

        std::vector<float> ResampleBilinear(const std::vector<float>& src,
                                            uint32_t srcResolution,
                                            uint32_t dstResolution)
        {
            std::vector<float> dst(static_cast<size_t>(dstResolution)
                                   * dstResolution);

            for (uint32_t y = 0; y < dstResolution; ++y)
            {
                for (uint32_t x = 0; x < dstResolution; ++x)
                {
                    const float u = (static_cast<float>(x) + 0.5f)
                            / static_cast<float>(dstResolution)
                            * static_cast<float>(srcResolution)
                        - 0.5f;
                    const float v = (static_cast<float>(y) + 0.5f)
                            / static_cast<float>(dstResolution)
                            * static_cast<float>(srcResolution)
                        - 0.5f;

                    const int x0 =
                        std::clamp(static_cast<int>(std::floor(u)), 0,
                                   static_cast<int>(srcResolution) - 1);
                    const int y0 =
                        std::clamp(static_cast<int>(std::floor(v)), 0,
                                   static_cast<int>(srcResolution) - 1);
                    const int x1 =
                        std::min(x0 + 1, static_cast<int>(srcResolution) - 1);
                    const int y1 =
                        std::min(y0 + 1, static_cast<int>(srcResolution) - 1);

                    const float fx =
                        std::clamp(u - static_cast<float>(x0), 0.0f, 1.0f);
                    const float fy =
                        std::clamp(v - static_cast<float>(y0), 0.0f, 1.0f);

                    const float h00 =
                        src[static_cast<size_t>(y0) * srcResolution + x0];
                    const float h10 =
                        src[static_cast<size_t>(y0) * srcResolution + x1];
                    const float h01 =
                        src[static_cast<size_t>(y1) * srcResolution + x0];
                    const float h11 =
                        src[static_cast<size_t>(y1) * srcResolution + x1];

                    const float top = glm::mix(h00, h10, fx);
                    const float bottom = glm::mix(h01, h11, fx);
                    dst[y * dstResolution + x] = glm::mix(top, bottom, fy);
                }
            }

            return dst;
        }
    } // namespace

    HydraulicErosionParams
    HydraulicErosionParams::ForWorkingResolution(uint32_t workingResolution,
                                                 uint32_t seed, float intensity)
    {
        HydraulicErosionParams params;
        params.seed = seed;
        params.workingResolution = workingResolution;

        const double texelCount = static_cast<double>(workingResolution)
            * static_cast<double>(workingResolution);
        params.numDroplets = static_cast<uint32_t>(
            texelCount * 4.0 * static_cast<double>(intensity));

        params.maxDropletLifetime = 64;
        params.inertia = 0.05f;
        params.initialWaterVolume = 1.0f;
        params.initialSpeed = 1.0f;
        params.sedimentCapacityFactor = 5.0f;
        params.minSedimentCapacity = 0.01f;
        params.erodeSpeed = 0.5f;
        params.depositSpeed = 0.15f;
        params.evaporateSpeed = 0.005f;
        params.gravity = 4.0f;
        params.erosionRadius = 5;

        return params;
    }

    void ApplyHydraulicErosion(std::vector<float>& heights, uint32_t resolution,
                               const HydraulicErosionParams& params)
    {
        if (resolution < 4)
        {
            return;
        }

        const uint32_t simResolution =
            (params.workingResolution == 0
             || params.workingResolution >= resolution)
            ? resolution
            : params.workingResolution;

        if (simResolution == resolution)
        {
            SimulateDroplets(heights, resolution, params);
            return;
        }

        std::vector<float> coarseHeights =
            ResampleBilinear(heights, resolution, simResolution);
        const std::vector<float> coarseOriginal = coarseHeights;

        SimulateDroplets(coarseHeights, simResolution, params);

        std::vector<float> coarseDelta(coarseHeights.size());
        for (size_t i = 0; i < coarseDelta.size(); ++i)
        {
            coarseDelta[i] = coarseHeights[i] - coarseOriginal[i];
        }

        const std::vector<float> fullDelta =
            ResampleBilinear(coarseDelta, simResolution, resolution);

        for (size_t i = 0; i < heights.size(); ++i)
        {
            heights[i] = std::clamp(heights[i] + fullDelta[i], 0.0f, 1.0f);
        }
    }
} // namespace raphEngine::terrain
