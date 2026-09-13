#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/map_builder.hpp>
#include <RaphEngine2/terrain/chunk_format.hpp>
#include <RaphEngine2/terrain/map.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <stb_image.h>
#include <stb_image_write.h>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    namespace
    {
        constexpr int kOverviewMaxResolution = 512;

        std::vector<uint16_t> ResampleHeightmap(const uint16_t* source,
                                                glm::ivec2 sourceDims,
                                                glm::ivec2 destDims)
        {
            std::vector<uint16_t> dest(static_cast<size_t>(destDims.x)
                                       * destDims.y);

            for (int y = 0; y < destDims.y; y++)
            {
                for (int x = 0; x < destDims.x; x++)
                {
                    const float u = (static_cast<float>(x) + 0.5f)
                            / static_cast<float>(destDims.x)
                            * static_cast<float>(sourceDims.x)
                        - 0.5f;
                    const float v = (static_cast<float>(y) + 0.5f)
                            / static_cast<float>(destDims.y)
                            * static_cast<float>(sourceDims.y)
                        - 0.5f;

                    const int x0 = std::clamp(static_cast<int>(std::floor(u)),
                                              0, sourceDims.x - 1);
                    const int y0 = std::clamp(static_cast<int>(std::floor(v)),
                                              0, sourceDims.y - 1);
                    const int x1 = std::min(x0 + 1, sourceDims.x - 1);
                    const int y1 = std::min(y0 + 1, sourceDims.y - 1);

                    const float fx =
                        std::clamp(u - static_cast<float>(x0), 0.0f, 1.0f);
                    const float fy =
                        std::clamp(v - static_cast<float>(y0), 0.0f, 1.0f);

                    const float h00 =
                        static_cast<float>(source[y0 * sourceDims.x + x0]);
                    const float h10 =
                        static_cast<float>(source[y0 * sourceDims.x + x1]);
                    const float h01 =
                        static_cast<float>(source[y1 * sourceDims.x + x0]);
                    const float h11 =
                        static_cast<float>(source[y1 * sourceDims.x + x1]);

                    const float top = h00 + (h10 - h00) * fx;
                    const float bottom = h01 + (h11 - h01) * fx;
                    const float value = top + (bottom - top) * fy;

                    dest[y * destDims.x + x] = static_cast<uint16_t>(
                        std::clamp(value + 0.5f, 0.0f, 65535.0f));
                }
            }

            return dest;
        }

        void WriteOverviewTexture(const std::vector<uint16_t>& fullHeights,
                                  uint32_t fullResolution,
                                  const fs::path& outputPath)
        {
            const int overviewRes = static_cast<int>(
                std::min<uint32_t>(fullResolution, kOverviewMaxResolution));

            const std::vector<uint16_t> resampled =
                ResampleHeightmap(fullHeights.data(),
                                  { static_cast<int>(fullResolution),
                                    static_cast<int>(fullResolution) },
                                  { overviewRes, overviewRes });

            std::vector<uint8_t> pixels(resampled.size());
            for (size_t i = 0; i < pixels.size(); i++)
            {
                pixels[i] = static_cast<uint8_t>(resampled[i] >> 8);
            }

            if (stbi_write_png(outputPath.string().c_str(), overviewRes,
                               overviewRes, 1, pixels.data(), overviewRes)
                == 0)
            {
                throw std::runtime_error(
                    "BuildMapFromHeightmap: failed to write overview texture "
                    + outputPath.string());
            }
        }
    } // namespace

    void BuildMapFromHeightmap(const fs::path& heightmapPath,
                               float sizeInMeters, float height,
                               const fs::path& outputDirectory)
    {
        if (sizeInMeters <= 0.0f)
        {
            throw std::runtime_error(
                "BuildMapFromHeightmap: sizeInMeters must be positive");
        }

        const auto sizeInMetersRounded =
            static_cast<uint32_t>(sizeInMeters + 0.5f);
        if (sizeInMetersRounded % kChunkResolution != 0)
        {
            throw std::runtime_error(
                "BuildMapFromHeightmap: sizeInMeters must be a multiple of "
                + std::to_string(kChunkResolution) + " meters (got "
                + std::to_string(sizeInMeters) + ")");
        }

        const uint32_t gridSize = sizeInMetersRounded / kChunkResolution;
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

        std::vector<uint16_t> fullHeights;
        try
        {
            fullHeights =
                ResampleHeightmap(sourcePixels, { sourceWidth, sourceHeight },
                                  { static_cast<int>(fullResolution),
                                    static_cast<int>(fullResolution) });
        }
        catch (...)
        {
            stbi_image_free(sourcePixels);
            throw;
        }
        stbi_image_free(sourcePixels);

        fs::create_directories(outputDirectory);

        const glm::vec2 worldHeightRange(0.0f, height);

        for (uint32_t gy = 0; gy < gridSize; gy++)
        {
            for (uint32_t gx = 0; gx < gridSize; gx++)
            {
                std::vector<uint16_t> chunkHeights(
                    static_cast<size_t>(kChunkResolution) * kChunkResolution);

                for (uint32_t y = 0; y < kChunkResolution; ++y)
                {
                    const uint32_t sourceY = gy * (kChunkResolution - 1) + y;
                    for (uint32_t x = 0; x < kChunkResolution; ++x)
                    {
                        const uint32_t sourceX =
                            gx * (kChunkResolution - 1) + x;
                        chunkHeights[y * kChunkResolution + x] =
                            fullHeights[sourceY * fullResolution + sourceX];
                    }
                }

                const glm::ivec2 gridCoord(static_cast<int>(gx),
                                           static_cast<int>(gy));
                const fs::path chunkPath =
                    Map::GetChunkFilePath(outputDirectory, gridCoord);
                detail::WriteChunkFile(chunkPath, gridCoord,
                                       ChunkSource::Imported, worldHeightRange,
                                       chunkHeights.data());
            }
        }

        WriteOverviewTexture(fullHeights, fullResolution,
                             Map::GetOverviewFilePath(outputDirectory));

        std::ofstream metaFile(Map::GetMetaFilePath(outputDirectory),
                               std::ios::binary | std::ios::trunc);
        if (!metaFile)
        {
            throw std::runtime_error(
                "BuildMapFromHeightmap: failed to open "
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

        metaFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
        if (!metaFile)
        {
            throw std::runtime_error(
                "BuildMapFromHeightmap: write failure for map metadata");
        }
    }
} // namespace raphEngine::terrain
