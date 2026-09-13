#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/chunk.hpp>

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace fs = std::filesystem;

namespace raphEngine::terrain::detail
{
    inline constexpr uint32_t kChunkFileMagic = 0x52544348;
    inline constexpr uint32_t kChunkFileVersion = 1;

#pragma pack(push, 1)
    struct ChunkFileHeader
    {
        uint32_t magic;
        uint32_t version;
        int32_t gridCoordX;
        int32_t gridCoordY;
        uint32_t resolution;
        uint8_t source;
        uint8_t reserved[3];
        float worldHeightMin;
        float worldHeightMax;
        uint32_t mipLevelCount;
    };
#pragma pack(pop)

    static_assert(sizeof(ChunkFileHeader) == 36,
                  "ChunkFileHeader layout must stay byte-exact for on-disk "
                  "compatibility");

    std::vector<glm::ivec2> ComputeMipLevelDims(uint32_t baseResolution);
    void FlipHeightRowsInPlace(uint16_t* heights, uint32_t width,
                               uint32_t height);

    std::vector<std::vector<HeightRange>>
    BuildMipPyramid(const uint16_t* baseHeights, uint32_t baseResolution);

    void WriteChunkFile(const fs::path& outputPath, glm::ivec2 gridCoord,
                        ChunkSource source, glm::vec2 worldHeightRange,
                        const uint16_t* heights);
} // namespace raphEngine::terrain::detail
