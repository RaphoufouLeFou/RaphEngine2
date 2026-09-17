#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/map_builder.hpp>
#include <RaphEngine2/terrain/chunk_format.hpp>
#include <RaphEngine2/terrain/map.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <execution>
#include <fstream>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <stb_image.h>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include "logger/logger.hpp"

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    namespace
    {
        constexpr int kOverviewMaxResolution = 512;

        constexpr uint32_t kOverviewFileMagic = 0x52544F56;
        constexpr uint32_t kOverviewFileVersion = 1;

        constexpr uint32_t kOverviewMaterialFileMagic = 0x52544F4D;
        constexpr uint32_t kOverviewMaterialFileVersion = 1;

        constexpr uint32_t kDefaultImageMaterialSeed = 918273645u;

        uint32_t DeriveMaterialSeed(uint32_t heightSeed)
        {
            return heightSeed ^ 0x9E3779B9u;
        }

#pragma pack(push, 1)
        struct OverviewFileHeader
        {
            uint32_t magic;
            uint32_t version;
            uint32_t width;
            uint32_t height;
        };

        struct OverviewMaterialFileHeader
        {
            uint32_t magic;
            uint32_t version;
            uint32_t width;
            uint32_t height;
        };
#pragma pack(pop)

        static_assert(sizeof(OverviewFileHeader) == 16,
                      "OverviewFileHeader layout must stay byte-exact for "
                      "on-disk compatibility");
        static_assert(sizeof(OverviewMaterialFileHeader) == 16,
                      "OverviewMaterialFileHeader layout must stay byte-exact "
                      "for on-disk compatibility");

        uint32_t ResolveGridSize(float sizeInMeters, const char* callerName)
        {
            if (sizeInMeters <= 0.0f)
            {
                throw std::runtime_error(std::string(callerName)
                                         + ": sizeInMeters must be positive");
            }

            const auto sizeInMetersRounded =
                static_cast<uint32_t>(sizeInMeters + 0.5f);
            if (sizeInMetersRounded % kChunkResolution != 0)
            {
                throw std::runtime_error(
                    std::string(callerName)
                    + ": sizeInMeters must be a multiple of "
                    + std::to_string(kChunkResolution) + " meters (got "
                    + std::to_string(sizeInMeters) + ")");
            }

            return sizeInMetersRounded / kChunkResolution;
        }

        uint16_t SampleSourceBilinear(const uint16_t* source, int sourceWidth,
                                      int sourceHeight, float fullResX,
                                      float fullResY, uint32_t fullResolution)
        {
            const float u = (fullResX + 0.5f)
                    / static_cast<float>(fullResolution)
                    * static_cast<float>(sourceWidth)
                - 0.5f;
            const float v = (fullResY + 0.5f)
                    / static_cast<float>(fullResolution)
                    * static_cast<float>(sourceHeight)
                - 0.5f;

            const int x0 =
                std::clamp(static_cast<int>(std::floor(u)), 0, sourceWidth - 1);
            const int y0 = std::clamp(static_cast<int>(std::floor(v)), 0,
                                      sourceHeight - 1);
            const int x1 = std::min(x0 + 1, sourceWidth - 1);
            const int y1 = std::min(y0 + 1, sourceHeight - 1);

            const float fx = std::clamp(u - static_cast<float>(x0), 0.0f, 1.0f);
            const float fy = std::clamp(v - static_cast<float>(y0), 0.0f, 1.0f);

            const float h00 = static_cast<float>(source[y0 * sourceWidth + x0]);
            const float h10 = static_cast<float>(source[y0 * sourceWidth + x1]);
            const float h01 = static_cast<float>(source[y1 * sourceWidth + x0]);
            const float h11 = static_cast<float>(source[y1 * sourceWidth + x1]);

            const float top = h00 + (h10 - h00) * fx;
            const float bottom = h01 + (h11 - h01) * fx;
            const float value = top + (bottom - top) * fy;

            return static_cast<uint16_t>(
                std::clamp(value + 0.5f, 0.0f, 65535.0f));
        }

        void WriteOverviewFileRaw(const std::vector<uint16_t>& overviewHeights,
                                  int overviewRes, const fs::path& outputPath)
        {
            std::ofstream outFile(outputPath,
                                  std::ios::binary | std::ios::trunc);
            if (!outFile)
            {
                throw std::runtime_error("WriteOverviewFileRaw: failed to open "
                                         + outputPath.string()
                                         + " for writing");
            }

            OverviewFileHeader header{};
            header.magic = kOverviewFileMagic;
            header.version = kOverviewFileVersion;
            header.width = static_cast<uint32_t>(overviewRes);
            header.height = static_cast<uint32_t>(overviewRes);

            outFile.write(reinterpret_cast<const char*>(&header),
                          sizeof(header));
            outFile.write(reinterpret_cast<const char*>(overviewHeights.data()),
                          static_cast<std::streamsize>(overviewHeights.size()
                                                       * sizeof(uint16_t)));

            if (!outFile)
            {
                throw std::runtime_error(
                    "WriteOverviewFileRaw: write failure for "
                    + outputPath.string());
            }
        }

        void WriteOverviewMaterialFileRaw(
            const std::vector<MaterialWeights>& overviewMaterials,
            int overviewRes, const fs::path& outputPath)
        {
            std::ofstream outFile(outputPath,
                                  std::ios::binary | std::ios::trunc);
            if (!outFile)
            {
                throw std::runtime_error(
                    "WriteOverviewMaterialFileRaw: failed to open "
                    + outputPath.string() + " for writing");
            }

            OverviewMaterialFileHeader header{};
            header.magic = kOverviewMaterialFileMagic;
            header.version = kOverviewMaterialFileVersion;
            header.width = static_cast<uint32_t>(overviewRes);
            header.height = static_cast<uint32_t>(overviewRes);

            outFile.write(reinterpret_cast<const char*>(&header),
                          sizeof(header));
            outFile.write(
                reinterpret_cast<const char*>(overviewMaterials.data()),
                static_cast<std::streamsize>(overviewMaterials.size()
                                             * sizeof(MaterialWeights)));

            if (!outFile)
            {
                throw std::runtime_error(
                    "WriteOverviewMaterialFileRaw: write failure for "
                    + outputPath.string());
            }
        }

        void
        WriteOverviewFileFromFullRes(const std::vector<uint16_t>& fullHeights,
                                     uint32_t fullResolution,
                                     const fs::path& outputPath)
        {
            const int overviewRes = static_cast<int>(
                std::min<uint32_t>(fullResolution, kOverviewMaxResolution));

            std::vector<uint16_t> overviewHeights(
                static_cast<size_t>(overviewRes) * overviewRes);
            for (int y = 0; y < overviewRes; ++y)
            {
                for (int x = 0; x < overviewRes; ++x)
                {
                    const float fullResX = (static_cast<float>(x) + 0.5f)
                        / static_cast<float>(overviewRes)
                        * static_cast<float>(fullResolution);
                    const float fullResY = (static_cast<float>(y) + 0.5f)
                        / static_cast<float>(overviewRes)
                        * static_cast<float>(fullResolution);

                    overviewHeights[y * overviewRes + x] = SampleSourceBilinear(
                        fullHeights.data(), static_cast<int>(fullResolution),
                        static_cast<int>(fullResolution), fullResX, fullResY,
                        fullResolution);
                }
            }

            WriteOverviewFileRaw(overviewHeights, overviewRes, outputPath);
        }

        // Downsamples a full-resolution MaterialWeights buffer to overview
        // resolution via bilinear interpolation on each channel
        // independently -- same technique as WriteOverviewFileFromFullRes,
        // just three bytes instead of one uint16.
        void WriteOverviewMaterialFromFullRes(
            const std::vector<MaterialWeights>& fullMaterials,
            uint32_t fullResolution, const fs::path& outputPath)
        {
            const int overviewRes = static_cast<int>(
                std::min<uint32_t>(fullResolution, kOverviewMaxResolution));

            std::vector<MaterialWeights> overviewMaterials(
                static_cast<size_t>(overviewRes) * overviewRes);
            for (int y = 0; y < overviewRes; ++y)
            {
                for (int x = 0; x < overviewRes; ++x)
                {
                    const float fullResX = (static_cast<float>(x) + 0.5f)
                        / static_cast<float>(overviewRes)
                        * static_cast<float>(fullResolution);
                    const float fullResY = (static_cast<float>(y) + 0.5f)
                        / static_cast<float>(overviewRes)
                        * static_cast<float>(fullResolution);

                    const float u = fullResX - 0.5f;
                    const float v = fullResY - 0.5f;
                    const int x0 =
                        std::clamp(static_cast<int>(std::floor(u)), 0,
                                   static_cast<int>(fullResolution) - 1);
                    const int y0 =
                        std::clamp(static_cast<int>(std::floor(v)), 0,
                                   static_cast<int>(fullResolution) - 1);
                    const int x1 =
                        std::min(x0 + 1, static_cast<int>(fullResolution) - 1);
                    const int y1 =
                        std::min(y0 + 1, static_cast<int>(fullResolution) - 1);
                    const float fx =
                        std::clamp(u - static_cast<float>(x0), 0.0f, 1.0f);
                    const float fy =
                        std::clamp(v - static_cast<float>(y0), 0.0f, 1.0f);

                    const MaterialWeights& w00 =
                        fullMaterials[static_cast<size_t>(y0) * fullResolution
                                      + x0];
                    const MaterialWeights& w10 =
                        fullMaterials[static_cast<size_t>(y0) * fullResolution
                                      + x1];
                    const MaterialWeights& w01 =
                        fullMaterials[static_cast<size_t>(y1) * fullResolution
                                      + x0];
                    const MaterialWeights& w11 =
                        fullMaterials[static_cast<size_t>(y1) * fullResolution
                                      + x1];

                    auto lerpChannel = [&](uint8_t a, uint8_t b, uint8_t c,
                                           uint8_t d) -> uint8_t {
                        const float top = glm::mix(static_cast<float>(a),
                                                   static_cast<float>(b), fx);
                        const float bottom = glm::mix(
                            static_cast<float>(c), static_cast<float>(d), fx);
                        return static_cast<uint8_t>(glm::mix(top, bottom, fy)
                                                    + 0.5f);
                    };

                    MaterialWeights result{};
                    result.rock =
                        lerpChannel(w00.rock, w10.rock, w01.rock, w11.rock);
                    result.snow =
                        lerpChannel(w00.snow, w10.snow, w01.snow, w11.snow);
                    result.dirt =
                        lerpChannel(w00.dirt, w10.dirt, w01.dirt, w11.dirt);
                    overviewMaterials[y * overviewRes + x] = result;
                }
            }

            WriteOverviewMaterialFileRaw(overviewMaterials, overviewRes,
                                         outputPath);
        }

        void WriteMetaFile(uint32_t gridSize, glm::vec2 worldHeightRange,
                           const fs::path& outputDirectory)
        {
            std::ofstream metaFile(Map::GetMetaFilePath(outputDirectory),
                                   std::ios::binary | std::ios::trunc);
            if (!metaFile)
            {
                throw std::runtime_error(
                    "WriteMetaFile: failed to open "
                    + Map::GetMetaFilePath(outputDirectory).string()
                    + " for writing");
            }

            detail::MapFileHeader header{};
            header.magic = detail::kMapFileMagic;
            header.version = detail::kMapFileVersion;
            header.gridSize = gridSize;
            header.chunkResolution = kChunkResolution;
            header.overviewHeightMin = worldHeightRange.x;
            header.overviewHeightMax = worldHeightRange.y;

            metaFile.write(reinterpret_cast<const char*>(&header),
                           sizeof(header));
            if (!metaFile)
            {
                throw std::runtime_error(
                    "WriteMetaFile: write failure for map metadata");
            }
        }

        void BuildAndWriteChunk(uint32_t gx, uint32_t gy,
                                const std::vector<uint16_t>& fullHeights,
                                uint32_t fullResolution,
                                glm::vec2 worldHeightRange,
                                const MaterialGenerator& materialGenerator,
                                std::vector<MaterialWeights>& fullMaterialsOut,
                                const fs::path& outputDirectory)
        {
            std::vector<uint16_t> chunkHeights(
                static_cast<size_t>(kChunkResolution) * kChunkResolution);
            std::vector<float> chunkHeightsMeters(chunkHeights.size());

            for (uint32_t y = 0; y < kChunkResolution; ++y)
            {
                const uint32_t sourceY = gy * (kChunkResolution - 1) + y;
                for (uint32_t x = 0; x < kChunkResolution; ++x)
                {
                    const uint32_t sourceX = gx * (kChunkResolution - 1) + x;
                    const uint16_t raw =
                        fullHeights[sourceY * fullResolution + sourceX];
                    chunkHeights[y * kChunkResolution + x] = raw;
                    chunkHeightsMeters[y * kChunkResolution + x] =
                        glm::mix(worldHeightRange.x, worldHeightRange.y,
                                 static_cast<float>(raw) / 65535.0f);
                }
            }

            const glm::ivec2 gridCoord(static_cast<int>(gx),
                                       static_cast<int>(gy));
            const glm::vec2 worldOrigin(
                static_cast<float>(gx) * (kChunkResolution - 1),
                static_cast<float>(gy) * (kChunkResolution - 1));
            const std::vector<MaterialWeights> materialWeights =
                materialGenerator.ComputeChunkWeights(
                    chunkHeightsMeters, kChunkResolution, worldOrigin);

            // Also copy this chunk's weights into the shared full-resolution
            // buffer, so the overview material map can be built by
            // downsampling it afterward without recomputing anything.
            // Each chunk writes a strictly disjoint region of
            // fullMaterialsOut, indexed exactly like fullHeights already
            // is -- safe from multiple chunks running concurrently.
            for (uint32_t y = 0; y < kChunkResolution; ++y)
            {
                const uint32_t sourceY = gy * (kChunkResolution - 1) + y;
                for (uint32_t x = 0; x < kChunkResolution; ++x)
                {
                    const uint32_t sourceX = gx * (kChunkResolution - 1) + x;
                    fullMaterialsOut[sourceY * fullResolution + sourceX] =
                        materialWeights[y * kChunkResolution + x];
                }
            }

            const fs::path chunkPath =
                Map::GetChunkFilePath(outputDirectory, gridCoord);
            detail::WriteChunkFile(chunkPath, gridCoord, ChunkSource::Imported,
                                   worldHeightRange, chunkHeights.data(),
                                   materialWeights.data());
        }

        void WriteMapFiles(const std::vector<uint16_t>& fullHeights,
                           uint32_t fullResolution, uint32_t gridSize,
                           glm::vec2 worldHeightRange,
                           uint32_t materialPatchSeed,
                           const fs::path& outputDirectory)
        {
            fs::create_directories(outputDirectory);

            const MaterialGenerator materialGenerator(materialPatchSeed,
                                                      worldHeightRange);

            // Real memory cost worth knowing: 3 bytes/texel on top of
            // fullHeights' own 2 bytes/texel -- roughly 1.5x's peak memory
            // for a large eager bake (e.g. ~1.1GB total instead of ~488MB
            // for a full 16km map). Acceptable for a one-time whole-world
            // bake; a leaner accumulate-directly-into-the-overview approach
            // exists but needs real synchronization across the parallel
            // per-chunk writes below, and hasn't been built.
            std::vector<MaterialWeights> fullMaterials(
                static_cast<size_t>(fullResolution) * fullResolution);

            std::vector<uint32_t> chunkIndices(static_cast<size_t>(gridSize)
                                               * gridSize);
            std::iota(chunkIndices.begin(), chunkIndices.end(), 0u);

            std::for_each(std::execution::par, chunkIndices.begin(),
                          chunkIndices.end(), [&](uint32_t index) {
                              const uint32_t gx = index % gridSize;
                              const uint32_t gy = index / gridSize;
                              BuildAndWriteChunk(
                                  gx, gy, fullHeights, fullResolution,
                                  worldHeightRange, materialGenerator,
                                  fullMaterials, outputDirectory);
                          });

            WriteOverviewFileFromFullRes(
                fullHeights, fullResolution,
                Map::GetOverviewFilePath(outputDirectory));
            WriteOverviewMaterialFromFullRes(
                fullMaterials, fullResolution,
                Map::GetOverviewMaterialFilePath(outputDirectory));
            WriteMetaFile(gridSize, worldHeightRange, outputDirectory);
        }
    } // namespace

    void BuildMapFromHeightmap(
        const fs::path& heightmapPath, float sizeInMeters, float height,
        const fs::path& outputDirectory,
        const std::optional<HydraulicErosionParams>& erosionParams)
    {
        const uint32_t gridSize =
            ResolveGridSize(sizeInMeters, "BuildMapFromHeightmap");
        const uint32_t fullResolution = gridSize * (kChunkResolution - 1) + 1;

        int sourceWidth = 0, sourceHeight = 0, sourceChannels = 0;
        uint16_t* sourcePixels =
            stbi_load_16(heightmapPath.string().c_str(), &sourceWidth,
                         &sourceHeight, &sourceChannels, 1);
        if (sourcePixels == nullptr)
        {
            throw std::runtime_error("BuildMapFromHeightmap: failed to load "
                                     + heightmapPath.string());
        }

        detail::FlipHeightRowsInPlace(sourcePixels,
                                      static_cast<uint32_t>(sourceWidth),
                                      static_cast<uint32_t>(sourceHeight));

        std::vector<float> normalizedHeights(static_cast<size_t>(fullResolution)
                                             * fullResolution);
        std::vector<uint32_t> rowIndices(fullResolution);
        std::iota(rowIndices.begin(), rowIndices.end(), 0u);

        std::for_each(std::execution::par, rowIndices.begin(), rowIndices.end(),
                      [&](uint32_t y) {
                          for (uint32_t x = 0; x < fullResolution; ++x)
                          {
                              const uint16_t raw = SampleSourceBilinear(
                                  sourcePixels, sourceWidth, sourceHeight,
                                  static_cast<float>(x), static_cast<float>(y),
                                  fullResolution);
                              normalizedHeights[y * fullResolution + x] =
                                  static_cast<float>(raw) / 65535.0f;
                          }
                      });

        stbi_image_free(sourcePixels);

        if (erosionParams)
        {
            ApplyHydraulicErosion(normalizedHeights, fullResolution,
                                  *erosionParams);
        }

        std::vector<uint16_t> fullHeights(normalizedHeights.size());
        std::for_each(std::execution::par, rowIndices.begin(), rowIndices.end(),
                      [&](uint32_t y) {
                          for (uint32_t x = 0; x < fullResolution; ++x)
                          {
                              const size_t i =
                                  static_cast<size_t>(y) * fullResolution + x;
                              fullHeights[i] = static_cast<uint16_t>(
                                  std::clamp(normalizedHeights[i], 0.0f, 1.0f)
                                      * 65535.0f
                                  + 0.5f);
                          }
                      });

        WriteMapFiles(fullHeights, fullResolution, gridSize,
                      glm::vec2(0.0f, height), kDefaultImageMaterialSeed,
                      outputDirectory);
    }

    void BuildMapFromNoise(
        const FractalNoiseParams& noiseParams, float sizeInMeters, float height,
        const fs::path& outputDirectory,
        const std::optional<HydraulicErosionParams>& erosionParams)
    {
        Logger::LogDebug("Building map from noise");
        const uint32_t gridSize =
            ResolveGridSize(sizeInMeters, "BuildMapFromNoise");
        const uint32_t fullResolution = gridSize * (kChunkResolution - 1) + 1;

        const PerlinNoise noise(noiseParams.seed);

        std::vector<float> normalizedHeights(static_cast<size_t>(fullResolution)
                                             * fullResolution);
        std::vector<uint32_t> rowIndices(fullResolution);
        std::iota(rowIndices.begin(), rowIndices.end(), 0u);

        std::for_each(std::execution::par, rowIndices.begin(), rowIndices.end(),
                      [&](uint32_t y) {
                          for (uint32_t x = 0; x < fullResolution; ++x)
                          {
                              const glm::vec2 worldXY(static_cast<float>(x),
                                                      static_cast<float>(y));
                              const float n = SampleFractalNoise(noise, worldXY,
                                                                 noiseParams);
                              normalizedHeights[y * fullResolution + x] =
                                  std::clamp(n * 0.5f + 0.5f, 0.0f, 1.0f);
                          }
                      });

        if (erosionParams)
        {
            ApplyHydraulicErosion(normalizedHeights, fullResolution,
                                  *erosionParams);
        }

        std::vector<uint16_t> fullHeights(normalizedHeights.size());
        std::for_each(std::execution::par, rowIndices.begin(), rowIndices.end(),
                      [&](uint32_t y) {
                          for (uint32_t x = 0; x < fullResolution; ++x)
                          {
                              const size_t i =
                                  static_cast<size_t>(y) * fullResolution + x;
                              fullHeights[i] = static_cast<uint16_t>(
                                  std::clamp(normalizedHeights[i], 0.0f, 1.0f)
                                      * 65535.0f
                                  + 0.5f);
                          }
                      });

        WriteMapFiles(fullHeights, fullResolution, gridSize,
                      glm::vec2(0.0f, height),
                      DeriveMaterialSeed(noiseParams.seed), outputDirectory);

        Logger::LogDebug("map was built from noise");
    }

    void BuildMapShellFromNoise(const FractalNoiseParams& noiseParams,
                                float sizeInMeters, float height,
                                const fs::path& outputDirectory)
    {
        const uint32_t gridSize =
            ResolveGridSize(sizeInMeters, "BuildMapShellFromNoise");
        const uint32_t fullResolution = gridSize * (kChunkResolution - 1) + 1;
        const int overviewRes = static_cast<int>(
            std::min<uint32_t>(fullResolution, kOverviewMaxResolution));

        const PerlinNoise noise(noiseParams.seed);

        std::vector<uint16_t> overviewHeights(static_cast<size_t>(overviewRes)
                                              * overviewRes);
        for (int y = 0; y < overviewRes; ++y)
        {
            for (int x = 0; x < overviewRes; ++x)
            {
                const float fx = (static_cast<float>(x) + 0.5f)
                    / static_cast<float>(overviewRes)
                    * static_cast<float>(fullResolution);
                const float fy = (static_cast<float>(y) + 0.5f)
                    / static_cast<float>(overviewRes)
                    * static_cast<float>(fullResolution);

                const float n =
                    SampleFractalNoise(noise, glm::vec2(fx, fy), noiseParams);
                const float normalized =
                    std::clamp(n * 0.5f + 0.5f, 0.0f, 1.0f);
                overviewHeights[y * overviewRes + x] =
                    static_cast<uint16_t>(normalized * 65535.0f + 0.5f);
            }
        }

        // Overview material: evaluated directly from the same continuous
        // noise used for height (via MaterialGenerator::ComputeWeightsAt),
        // never a materialized full-resolution buffer -- keeps this shell
        // path bounded by overview resolution regardless of world size,
        // same as height's own overview construction above.
        const MaterialGenerator materialGenerator(
            DeriveMaterialSeed(noiseParams.seed), glm::vec2(0.0f, height));
        std::vector<MaterialWeights> overviewMaterials(
            static_cast<size_t>(overviewRes) * overviewRes);

        tbb::parallel_for(
            uint32_t(0), static_cast<uint32_t>(overviewRes), [&](uint32_t y) {
                for (int x = 0; x < overviewRes; ++x)
                {
                    const float fx = (static_cast<float>(x) + 0.5f)
                        / static_cast<float>(overviewRes)
                        * static_cast<float>(fullResolution);
                    const float fy = (static_cast<float>(y) + 0.5f)
                        / static_cast<float>(overviewRes)
                        * static_cast<float>(fullResolution);

                    auto heightAt = [&](glm::vec2 worldXY) -> float {
                        const float n =
                            SampleFractalNoise(noise, worldXY, noiseParams);
                        const float normalized =
                            std::clamp(n * 0.5f + 0.5f, 0.0f, 1.0f);
                        return normalized * height;
                    };

                    overviewMaterials[y * overviewRes + x] =
                        materialGenerator.ComputeWeightsAt(glm::vec2(fx, fy),
                                                           heightAt);
                }
            });

        fs::create_directories(outputDirectory);
        WriteOverviewFileRaw(overviewHeights, overviewRes,
                             Map::GetOverviewFilePath(outputDirectory));
        WriteOverviewMaterialFileRaw(
            overviewMaterials, overviewRes,
            Map::GetOverviewMaterialFilePath(outputDirectory));
        WriteMetaFile(gridSize, glm::vec2(0.0f, height), outputDirectory);
    }

    void BuildMapShellFromHeightmap(const fs::path& heightmapPath,
                                    float sizeInMeters, float height,
                                    const fs::path& outputDirectory)
    {
        const uint32_t gridSize =
            ResolveGridSize(sizeInMeters, "BuildMapShellFromHeightmap");
        const uint32_t fullResolution = gridSize * (kChunkResolution - 1) + 1;
        const int overviewRes = static_cast<int>(
            std::min<uint32_t>(fullResolution, kOverviewMaxResolution));

        int sourceWidth = 0, sourceHeight = 0, sourceChannels = 0;
        uint16_t* sourcePixels =
            stbi_load_16(heightmapPath.string().c_str(), &sourceWidth,
                         &sourceHeight, &sourceChannels, 1);
        if (sourcePixels == nullptr)
        {
            throw std::runtime_error(
                "BuildMapShellFromHeightmap: failed to load "
                + heightmapPath.string());
        }

        detail::FlipHeightRowsInPlace(sourcePixels,
                                      static_cast<uint32_t>(sourceWidth),
                                      static_cast<uint32_t>(sourceHeight));

        std::vector<uint16_t> overviewHeights(static_cast<size_t>(overviewRes)
                                              * overviewRes);
        for (int y = 0; y < overviewRes; ++y)
        {
            for (int x = 0; x < overviewRes; ++x)
            {
                const float fx = (static_cast<float>(x) + 0.5f)
                    / static_cast<float>(overviewRes)
                    * static_cast<float>(fullResolution);
                const float fy = (static_cast<float>(y) + 0.5f)
                    / static_cast<float>(overviewRes)
                    * static_cast<float>(fullResolution);

                overviewHeights[y * overviewRes + x] =
                    SampleSourceBilinear(sourcePixels, sourceWidth,
                                         sourceHeight, fx, fy, fullResolution);
            }
        }

        const MaterialGenerator materialGenerator(kDefaultImageMaterialSeed,
                                                  glm::vec2(0.0f, height));
        std::vector<MaterialWeights> overviewMaterials(
            static_cast<size_t>(overviewRes) * overviewRes);

        tbb::parallel_for(
            uint32_t(0), static_cast<uint32_t>(overviewRes), [&](uint32_t y) {
                for (int x = 0; x < overviewRes; ++x)
                {
                    const float fx = (static_cast<float>(x) + 0.5f)
                        / static_cast<float>(overviewRes)
                        * static_cast<float>(fullResolution);
                    const float fy = (static_cast<float>(y) + 0.5f)
                        / static_cast<float>(overviewRes)
                        * static_cast<float>(fullResolution);

                    auto heightAt = [&](glm::vec2 worldXY) -> float {
                        const uint16_t raw = SampleSourceBilinear(
                            sourcePixels, sourceWidth, sourceHeight, worldXY.x,
                            worldXY.y, fullResolution);
                        return (static_cast<float>(raw) / 65535.0f) * height;
                    };

                    overviewMaterials[y * overviewRes + x] =
                        materialGenerator.ComputeWeightsAt(glm::vec2(fx, fy),
                                                           heightAt);
                }
            });

        stbi_image_free(sourcePixels);

        fs::create_directories(outputDirectory);
        WriteOverviewFileRaw(overviewHeights, overviewRes,
                             Map::GetOverviewFilePath(outputDirectory));
        WriteOverviewMaterialFileRaw(
            overviewMaterials, overviewRes,
            Map::GetOverviewMaterialFilePath(outputDirectory));
        WriteMetaFile(gridSize, glm::vec2(0.0f, height), outputDirectory);
    }

    NoiseChunkGenerator::NoiseChunkGenerator(const FractalNoiseParams& params,
                                             float height)
        : noise_(params.seed)
        , materialGenerator_(DeriveMaterialSeed(params.seed),
                             glm::vec2(0.0f, height))
        , params_(params)
        , height_(height)
    {}

    void NoiseChunkGenerator::SetParams(const FractalNoiseParams& params,
                                        float height)
    {
        noise_ = PerlinNoise(params.seed);
        materialGenerator_ = MaterialGenerator(DeriveMaterialSeed(params.seed),
                                               glm::vec2(0.0f, height));
        params_ = params;
        height_ = height;
    }

    void NoiseChunkGenerator::operator()(glm::ivec2 gridCoord,
                                         const fs::path& chunkPath) const
    {
        std::vector<uint16_t> chunkHeights(static_cast<size_t>(kChunkResolution)
                                           * kChunkResolution);
        std::vector<float> chunkHeightsMeters(chunkHeights.size());

        tbb::parallel_for(
            tbb::blocked_range<uint32_t>(0, kChunkResolution),
            [&](const tbb::blocked_range<uint32_t>& range) {
                for (uint32_t ly = range.begin(); ly != range.end(); ++ly)
                {
                    const uint32_t sourceY = static_cast<uint32_t>(gridCoord.y)
                            * (kChunkResolution - 1)
                        + ly;
                    for (uint32_t lx = 0; lx < kChunkResolution; ++lx)
                    {
                        const uint32_t sourceX =
                            static_cast<uint32_t>(gridCoord.x)
                                * (kChunkResolution - 1)
                            + lx;
                        const glm::vec2 worldXY(static_cast<float>(sourceX),
                                                static_cast<float>(sourceY));
                        const float n =
                            SampleFractalNoise(noise_, worldXY, params_);
                        const float normalized =
                            std::clamp(n * 0.5f + 0.5f, 0.0f, 1.0f);
                        chunkHeights[ly * kChunkResolution + lx] =
                            static_cast<uint16_t>(normalized * 65535.0f + 0.5f);
                        chunkHeightsMeters[ly * kChunkResolution + lx] =
                            normalized * height_;
                    }
                }
            });

        const glm::vec2 worldOrigin(
            static_cast<float>(gridCoord.x) * (kChunkResolution - 1),
            static_cast<float>(gridCoord.y) * (kChunkResolution - 1));
        const std::vector<MaterialWeights> materialWeights =
            materialGenerator_.ComputeChunkWeights(
                chunkHeightsMeters, kChunkResolution, worldOrigin);

        detail::WriteChunkFile(chunkPath, gridCoord, ChunkSource::Imported,
                               glm::vec2(0.0f, height_), chunkHeights.data(),
                               materialWeights.data());
    }

    ImageChunkGenerator::ImageChunkGenerator(const fs::path& heightmapPath,
                                             float sizeInMeters, float height)
        : height_(height)
        , materialGenerator_(kDefaultImageMaterialSeed, glm::vec2(0.0f, height))
    {
        const uint32_t gridSize =
            ResolveGridSize(sizeInMeters, "ImageChunkGenerator");
        fullResolution_ = gridSize * (kChunkResolution - 1) + 1;

        int channels = 0;
        sourcePixels_ =
            stbi_load_16(heightmapPath.string().c_str(), &sourceWidth_,
                         &sourceHeight_, &channels, 1);
        if (sourcePixels_ == nullptr)
        {
            throw std::runtime_error("ImageChunkGenerator: failed to load "
                                     + heightmapPath.string());
        }

        detail::FlipHeightRowsInPlace(sourcePixels_,
                                      static_cast<uint32_t>(sourceWidth_),
                                      static_cast<uint32_t>(sourceHeight_));
    }

    ImageChunkGenerator::~ImageChunkGenerator()
    {
        if (sourcePixels_ != nullptr)
        {
            stbi_image_free(sourcePixels_);
        }
    }

    void ImageChunkGenerator::operator()(glm::ivec2 gridCoord,
                                         const fs::path& chunkPath) const
    {
        std::vector<uint16_t> chunkHeights(static_cast<size_t>(kChunkResolution)
                                           * kChunkResolution);
        std::vector<float> chunkHeightsMeters(chunkHeights.size());

        for (uint32_t ly = 0; ly < kChunkResolution; ++ly)
        {
            const uint32_t fullResY =
                static_cast<uint32_t>(gridCoord.y) * (kChunkResolution - 1)
                + ly;
            for (uint32_t lx = 0; lx < kChunkResolution; ++lx)
            {
                const uint32_t fullResX =
                    static_cast<uint32_t>(gridCoord.x) * (kChunkResolution - 1)
                    + lx;
                const uint16_t raw = SampleSourceBilinear(
                    sourcePixels_, sourceWidth_, sourceHeight_,
                    static_cast<float>(fullResX), static_cast<float>(fullResY),
                    fullResolution_);
                chunkHeights[ly * kChunkResolution + lx] = raw;
                chunkHeightsMeters[ly * kChunkResolution + lx] =
                    (static_cast<float>(raw) / 65535.0f) * height_;
            }
        }

        const glm::vec2 worldOrigin(
            static_cast<float>(gridCoord.x) * (kChunkResolution - 1),
            static_cast<float>(gridCoord.y) * (kChunkResolution - 1));
        const std::vector<MaterialWeights> materialWeights =
            materialGenerator_.ComputeChunkWeights(
                chunkHeightsMeters, kChunkResolution, worldOrigin);

        detail::WriteChunkFile(chunkPath, gridCoord, ChunkSource::Imported,
                               glm::vec2(0.0f, height_), chunkHeights.data(),
                               materialWeights.data());
    }
} // namespace raphEngine::terrain
