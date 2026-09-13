#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/chunk_format.hpp>

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace raphEngine::terrain::detail
{
    std::vector<glm::ivec2> ComputeMipLevelDims(uint32_t baseResolution)
    {
        std::vector<glm::ivec2> dims;
        uint32_t current = baseResolution;
        while (current > 1)
        {
            current = (current + 1) / 2;
            dims.emplace_back(static_cast<int>(current),
                              static_cast<int>(current));
        }
        return dims;
    }

    namespace
    {
        std::vector<HeightRange> BuildFirstMipLevel(const uint16_t* baseHeights,
                                                    glm::ivec2 baseDims,
                                                    glm::ivec2 destDims)
        {
            std::vector<HeightRange> level(static_cast<size_t>(destDims.x)
                                           * destDims.y);

            for (int y = 0; y < destDims.y; y++)
            {
                for (int x = 0; x < destDims.x; x++)
                {
                    uint16_t minVal = 0xFFFF;
                    uint16_t maxVal = 0;

                    for (int dy = 0; dy < 2; dy++)
                    {
                        for (int dx = 0; dx < 2; dx++)
                        {
                            const int sx = std::min(x * 2 + dx, baseDims.x - 1);
                            const int sy = std::min(y * 2 + dy, baseDims.y - 1);
                            const uint16_t value =
                                baseHeights[sy * baseDims.x + sx];
                            minVal = std::min(minVal, value);
                            maxVal = std::max(maxVal, value);
                        }
                    }

                    level[y * destDims.x + x] = HeightRange{ minVal, maxVal };
                }
            }

            return level;
        }

        std::vector<HeightRange>
        BuildNextMipLevel(const std::vector<HeightRange>& previousLevel,
                          glm::ivec2 previousDims, glm::ivec2 destDims)
        {
            std::vector<HeightRange> level(static_cast<size_t>(destDims.x)
                                           * destDims.y);

            for (int y = 0; y < destDims.y; y++)
            {
                for (int x = 0; x < destDims.x; x++)
                {
                    uint16_t minVal = 0xFFFF;
                    uint16_t maxVal = 0;

                    for (int dy = 0; dy < 2; dy++)
                    {
                        for (int dx = 0; dx < 2; dx++)
                        {
                            const int sx =
                                std::min(x * 2 + dx, previousDims.x - 1);
                            const int sy =
                                std::min(y * 2 + dy, previousDims.y - 1);
                            const HeightRange& src =
                                previousLevel[sy * previousDims.x + sx];
                            minVal = std::min(minVal, src.min);
                            maxVal = std::max(maxVal, src.max);
                        }
                    }

                    level[y * destDims.x + x] = HeightRange{ minVal, maxVal };
                }
            }

            return level;
        }
    } // namespace

    void FlipHeightRowsInPlace(uint16_t* heights, uint32_t width,
                               uint32_t height)
    {
        for (uint32_t y = 0; y < height / 2; ++y)
        {
            uint16_t* rowA = heights + static_cast<size_t>(y) * width;
            uint16_t* rowB =
                heights + static_cast<size_t>(height - 1 - y) * width;
            std::swap_ranges(rowA, rowA + width, rowB);
        }
    }

    std::vector<std::vector<HeightRange>>
    BuildMipPyramid(const uint16_t* baseHeights, uint32_t baseResolution)
    {
        const std::vector<glm::ivec2> mipDims =
            ComputeMipLevelDims(baseResolution);

        std::vector<std::vector<HeightRange>> mipLevels;
        mipLevels.reserve(mipDims.size());

        glm::ivec2 previousDims(static_cast<int>(baseResolution),
                                static_cast<int>(baseResolution));
        for (size_t i = 0; i < mipDims.size(); i++)
        {
            if (i == 0)
            {
                mipLevels.push_back(
                    BuildFirstMipLevel(baseHeights, previousDims, mipDims[i]));
            }
            else
            {
                mipLevels.push_back(BuildNextMipLevel(
                    mipLevels[i - 1], previousDims, mipDims[i]));
            }
            previousDims = mipDims[i];
        }

        return mipLevels;
    }

    void WriteChunkFile(const fs::path& outputPath, glm::ivec2 gridCoord,
                        ChunkSource source, glm::vec2 worldHeightRange,
                        const uint16_t* heights)
    {
        const std::vector<std::vector<HeightRange>> mipLevels =
            BuildMipPyramid(heights, kChunkResolution);

        std::ofstream outFile(outputPath, std::ios::binary | std::ios::trunc);
        if (!outFile)
        {
            throw std::runtime_error("WriteChunkFile: failed to open "
                                     + outputPath.string() + " for writing");
        }

        ChunkFileHeader header{};
        header.magic = kChunkFileMagic;
        header.version = kChunkFileVersion;
        header.gridCoordX = gridCoord.x;
        header.gridCoordY = gridCoord.y;
        header.resolution = kChunkResolution;
        header.source = static_cast<uint8_t>(source);
        header.worldHeightMin = worldHeightRange.x;
        header.worldHeightMax = worldHeightRange.y;
        header.mipLevelCount = static_cast<uint32_t>(mipLevels.size());

        outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
        outFile.write(reinterpret_cast<const char*>(heights),
                      static_cast<std::streamsize>(kChunkResolution)
                          * kChunkResolution * sizeof(uint16_t));

        for (const std::vector<HeightRange>& level : mipLevels)
        {
            outFile.write(reinterpret_cast<const char*>(level.data()),
                          static_cast<std::streamsize>(level.size()
                                                       * sizeof(HeightRange)));
        }

        if (!outFile)
        {
            throw std::runtime_error("WriteChunkFile: write failure for "
                                     + outputPath.string());
        }
    }
} // namespace raphEngine::terrain::detail
