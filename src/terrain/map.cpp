#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/chunk.hpp>
#include <RaphEngine2/terrain/map.hpp>

#include <algorithm>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{

    std::unique_ptr<Map> Map::instance = nullptr;

    Map* Map::GetInstace()
    {
        return instance.get();
    }

    void Map::FromFile(const fs::path& p)
    {
        instance = std::make_unique<Map>();
        instance->Load(p);
    }

    fs::path Map::GetChunkFilePath(const fs::path& rootDirectory,
                                   glm::ivec2 gridCoord)
    {
        return rootDirectory
            / ("chunk_" + std::to_string(gridCoord.x) + "_"
               + std::to_string(gridCoord.y) + ".chunk");
    }

    fs::path Map::GetMetaFilePath(const fs::path& rootDirectory)
    {
        return rootDirectory / "map.meta";
    }

    fs::path Map::GetOverviewFilePath(const fs::path& rootDirectory)
    {
        return rootDirectory / "overview.heightmap";
    }
    namespace
    {
        constexpr uint32_t kOverviewFileMagic = 0x52544F56; // "RTOV"
        constexpr uint32_t kOverviewFileVersion = 1;

#pragma pack(push, 1)
        struct OverviewFileHeader
        {
            uint32_t magic;
            uint32_t version;
            uint32_t width;
            uint32_t height;
        };
#pragma pack(pop)
    } // namespace
    void Map::OverviewHeightMap::Load(const fs::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error(
                "Map::OverviewHeightMap::Load: failed to open "
                + path.string());
        }

        OverviewFileHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file || header.magic != kOverviewFileMagic
            || header.version != kOverviewFileVersion)
        {
            throw std::runtime_error(
                "Map::OverviewHeightMap::Load: malformed overview file "
                + path.string());
        }

        width_ = static_cast<int>(header.width);
        height_ = static_cast<int>(header.height);

        heights_.resize(static_cast<size_t>(width_) * height_);
        file.read(
            reinterpret_cast<char*>(heights_.data()),
            static_cast<std::streamsize>(heights_.size() * sizeof(uint16_t)));

        if (!file)
        {
            throw std::runtime_error(
                "Map::OverviewHeightMap::Load: truncated overview file "
                + path.string());
        }
    }

    void Map::OverviewHeightMap::Release()
    {
        if (raw_.data != nullptr)
        {
            graphics::TextureLoader::getInstance()->free_raw(raw_);
            raw_ = {};
        }
    }

    float
    Map::OverviewHeightMap::SampleNormalizedHeight(glm::vec2 normalizedUV) const
    {
        const glm::vec2 clamped =
            glm::clamp(normalizedUV, glm::vec2(0.0f), glm::vec2(1.0f));

        const glm::vec2 texelCoord = clamped
                * glm::vec2(static_cast<float>(width_),
                            static_cast<float>(height_))
            - glm::vec2(0.5f);

        const glm::vec2 base = glm::floor(texelCoord);
        const glm::vec2 frac = texelCoord - base;

        const int x0 = std::clamp(static_cast<int>(base.x), 0, width_ - 1);
        const int x1 = std::clamp(static_cast<int>(base.x) + 1, 0, width_ - 1);
        const int y0 = std::clamp(static_cast<int>(base.y), 0, height_ - 1);
        const int y1 = std::clamp(static_cast<int>(base.y) + 1, 0, height_ - 1);

        auto sampleTexel = [&](int x, int y) -> float {
            return static_cast<float>(
                       heights_[static_cast<size_t>(y) * width_ + x])
                / 65535.0f;
        };

        const float h00 = sampleTexel(x0, y0);
        const float h10 = sampleTexel(x1, y0);
        const float h01 = sampleTexel(x0, y1);
        const float h11 = sampleTexel(x1, y1);

        const float top = glm::mix(h00, h10, frac.x);
        const float bottom = glm::mix(h01, h11, frac.x);
        return glm::mix(top, bottom, frac.y);
    }

    void Map::Load(const fs::path& p)
    {
        rootDirectory_ = p;
        chunks_.clear();

        std::ifstream metaFile(GetMetaFilePath(p), std::ios::binary);
        if (!metaFile)
        {
            throw std::runtime_error("Map::Load: failed to open "
                                     + GetMetaFilePath(p).string());
        }

        detail::MapFileHeader header{};
        metaFile.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!metaFile || header.magic != detail::kMapFileMagic
            || header.version != detail::kMapFileVersion)
        {
            throw std::runtime_error("Map::Load: malformed map metadata in "
                                     + GetMetaFilePath(p).string());
        }

        if (header.chunkResolution != kChunkResolution)
        {
            throw std::runtime_error("Map::Load: chunk resolution mismatch in "
                                     + GetMetaFilePath(p).string());
        }

        if (header.gridSize == 0)
        {
            throw std::runtime_error("Map::Load: grid size must be non-zero in "
                                     + GetMetaFilePath(p).string());
        }

        gridSize_ = header.gridSize;
        overviewHeightRange_ = { header.overviewHeightMin,
                                 header.overviewHeightMax };
        overviewHeightMap_.Load(GetOverviewFilePath(p));

        chunks_.resize(static_cast<size_t>(gridSize_) * gridSize_);
        for (uint32_t y = 0; y < gridSize_; y++)
        {
            for (uint32_t x = 0; x < gridSize_; x++)
            {
                const glm::ivec2 coord(static_cast<int>(x),
                                       static_cast<int>(y));
                chunks_[GetChunkIndex(coord)].SetGridCoord(coord);
            }
        }

        Logger::LogDebug("header tells there is ", chunks_.size(), " chunks");
    }

    void Map::Save(const fs::path& p)
    {
        for (Chunk& chunk : chunks_)
        {
            if (chunk.GetState() == ChunkState::Resident && chunk.IsDirty())
            {
                chunk.Save(GetChunkFilePath(p, chunk.GetGridCoord()));
            }
        }
    }

    void Map::UpdateStreaming(glm::vec3 viewerWorldPosition,
                              float streamingRadiusMeters,
                              uint32_t maxLoadsPerCall)
    {
        constexpr float kUnloadHysteresisFactor = 1.15f;

        const glm::vec2 viewerXY(viewerWorldPosition.x, viewerWorldPosition.y);
        const float loadRadiusSq =
            streamingRadiusMeters * streamingRadiusMeters;
        const float unloadRadius =
            streamingRadiusMeters * kUnloadHysteresisFactor;
        const float unloadRadiusSq = unloadRadius * unloadRadius;

        uint32_t loadsThisCall = 0;

        for (uint32_t y = 0; y < gridSize_; ++y)
        {
            for (uint32_t x = 0; x < gridSize_; ++x)
            {
                const glm::ivec2 coord(static_cast<int>(x),
                                       static_cast<int>(y));
                Chunk& chunk = chunks_[GetChunkIndex(coord)];

                const glm::vec2 chunkCenter = GridCoordToWorldOrigin(coord)
                    + glm::vec2(static_cast<float>(kChunkResolution) * 0.5f);
                const glm::vec2 diff = chunkCenter - viewerXY;
                const float distSq = glm::dot(diff, diff);

                const bool isResident =
                    chunk.GetState() == ChunkState::Resident;

                if (!isResident && distSq <= loadRadiusSq)
                {
                    if (loadsThisCall >= maxLoadsPerCall)
                    {
                        continue;
                    }

                    const fs::path chunkPath =
                        GetChunkFilePath(rootDirectory_, coord);
                    if (fs::exists(chunkPath))
                    {
                        chunk.Load(chunkPath);
                        ++loadsThisCall;
                    }
                }
                else if (isResident && distSq > unloadRadiusSq)
                {
                    if (chunk.IsDirty())
                    {
                        chunk.Save(GetChunkFilePath(rootDirectory_, coord));
                    }
                    chunk.Unload();
                }
            }
        }
    }

    float Map::GetHeightAt(glm::vec2 worldPositionXY) const
    {
        const glm::ivec2 coord = WorldToGridCoord(worldPositionXY);
        if (IsValidGridCoord(coord) && IsChunkResident(coord))
        {
            const glm::vec2 localPos =
                WorldToChunkLocal(worldPositionXY, coord);
            return chunks_[GetChunkIndex(coord)].GetHeightAt(localPos);
        }

        return SampleOverviewHeightAt(worldPositionXY);
    }

    glm::vec2 Map::GridCoordToWorldOrigin(glm::ivec2 gridCoord) const
    {
        const float halfWorldSize = GetWorldSizeMeters() * 0.5f;
        return glm::vec2(gridCoord) * static_cast<float>(kChunkResolution)
            - glm::vec2(halfWorldSize);
    }

    glm::ivec2 Map::WorldToGridCoord(glm::vec2 worldPositionXY) const
    {
        const float halfWorldSize = GetWorldSizeMeters() * 0.5f;
        return glm::ivec2(
            glm::floor((worldPositionXY + glm::vec2(halfWorldSize))
                       / static_cast<float>(kChunkResolution)));
    }

    glm::vec2 Map::WorldToChunkLocal(glm::vec2 worldPositionXY,
                                     glm::ivec2 gridCoord) const
    {
        return worldPositionXY - GridCoordToWorldOrigin(gridCoord);
    }

    float Map::SampleOverviewHeightAt(glm::vec2 worldPositionXY) const
    {
        const float halfWorldSize = GetWorldSizeMeters() * 0.5f;
        const glm::vec2 normalized =
            (worldPositionXY + glm::vec2(halfWorldSize)) / GetWorldSizeMeters();
        const float t = overviewHeightMap_.SampleNormalizedHeight(normalized);
        return glm::mix(overviewHeightRange_.x, overviewHeightRange_.y, t);
    }

    Chunk* Map::GetChunkAt(glm::ivec2 gridCoord)
    {
        if (!IsValidGridCoord(gridCoord))
        {
            return nullptr;
        }
        return &chunks_[GetChunkIndex(gridCoord)];
    }

    const Chunk* Map::GetChunkAt(glm::ivec2 gridCoord) const
    {
        if (!IsValidGridCoord(gridCoord))
        {
            return nullptr;
        }
        return &chunks_[GetChunkIndex(gridCoord)];
    }

    Chunk* Map::GetChunkContainingWorldPosition(glm::vec2 worldPositionXY)
    {
        return GetChunkAt(WorldToGridCoord(worldPositionXY));
    }

    bool Map::IsChunkResident(glm::ivec2 gridCoord) const
    {
        if (!IsValidGridCoord(gridCoord))
        {
            return false;
        }
        return chunks_[GetChunkIndex(gridCoord)].GetState()
            == ChunkState::Resident;
    }

    bool Map::IsValidGridCoord(glm::ivec2 gridCoord) const noexcept
    {
        return gridCoord.x >= 0 && gridCoord.y >= 0
            && gridCoord.x < static_cast<int>(gridSize_)
            && gridCoord.y < static_cast<int>(gridSize_);
    }

    size_t Map::GetChunkIndex(glm::ivec2 gridCoord) const noexcept
    {
        return static_cast<size_t>(gridCoord.y) * gridSize_
            + static_cast<size_t>(gridCoord.x);
    }

} // namespace raphEngine::terrain
