#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/chunk.hpp>
#include <RaphEngine2/graphics/texture_loader.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <glm/glm.hpp>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    namespace detail
    {
#pragma pack(push, 1)
        struct MapFileHeader
        {
            uint32_t magic;
            uint32_t version;
            uint32_t gridSize;
            uint32_t chunkResolution;
            float overviewHeightMin;
            float overviewHeightMax;
        };
#pragma pack(pop)

        static_assert(sizeof(MapFileHeader) == 24,
                      "MapFileHeader layout must stay byte-exact for on-disk "
                      "compatibility");

        inline constexpr uint32_t kMapFileMagic = 0x52544D50;
        inline constexpr uint32_t kMapFileVersion = 1;
    } // namespace detail

    class RAPHENGINE_API Map
    {
    public:
        Map() = default;
        ~Map() = default;

        Map(const Map&) = delete;
        Map& operator=(const Map&) = delete;
        Map(Map&&) = default;
        Map& operator=(Map&&) = default;

        static void FromFile(const fs::path&);

        void Load(const fs::path&);
        void Save(const fs::path&);

        void UpdateStreaming(glm::vec3 viewerWorldPosition,
                             float streamingRadiusMeters,
                             uint32_t maxLoadsPerCall = 4);

        float GetHeightAt(glm::vec2 worldPositionXY) const;

        Chunk* GetChunkAt(glm::ivec2 gridCoord);
        const Chunk* GetChunkAt(glm::ivec2 gridCoord) const;
        Chunk* GetChunkContainingWorldPosition(glm::vec2 worldPositionXY);

        glm::vec2 GridCoordToWorldOrigin(glm::ivec2 gridCoord) const;
        bool IsChunkResident(glm::ivec2 gridCoord) const;
        bool IsValidGridCoord(glm::ivec2 gridCoord) const noexcept;

        std::span<const Chunk> GetChunks() const noexcept
        {
            return chunks_;
        }

        glm::ivec2 GetGridSize() const noexcept
        {
            return { static_cast<int>(gridSize_), static_cast<int>(gridSize_) };
        }
        float GetWorldSizeMeters() const noexcept
        {
            return static_cast<float>(gridSize_ * kChunkResolution);
        }

        static fs::path GetChunkFilePath(const fs::path& rootDirectory,
                                         glm::ivec2 gridCoord);
        static fs::path GetMetaFilePath(const fs::path& rootDirectory);
        static fs::path GetOverviewFilePath(const fs::path& rootDirectory);

        static Map* GetInstace();

    private:
        static std::unique_ptr<Map> instance;

        class OverviewHeightMap
        {
        public:
            OverviewHeightMap() = default;
            ~OverviewHeightMap();

            OverviewHeightMap(const OverviewHeightMap&) = delete;
            OverviewHeightMap& operator=(const OverviewHeightMap&) = delete;
            OverviewHeightMap(OverviewHeightMap&&) noexcept;
            OverviewHeightMap& operator=(OverviewHeightMap&&) noexcept;

            void Load(const fs::path& path);
            float SampleNormalizedHeight(glm::vec2 normalizedUV) const;
            bool IsLoaded() const noexcept
            {
                return raw_.data != nullptr;
            }

        private:
            void Release();

            graphics::TextureLoader::RawTexture raw_{};
        };

        glm::ivec2 WorldToGridCoord(glm::vec2 worldPositionXY) const;
        glm::vec2 WorldToChunkLocal(glm::vec2 worldPositionXY,
                                    glm::ivec2 gridCoord) const;
        size_t GetChunkIndex(glm::ivec2 gridCoord) const noexcept;
        float SampleOverviewHeightAt(glm::vec2 worldPositionXY) const;

        std::vector<Chunk> chunks_;
        OverviewHeightMap overviewHeightMap_;

        fs::path rootDirectory_;
        glm::vec2 overviewHeightRange_{ 0.0f, 0.0f };
        uint32_t gridSize_ = 0;
    };
} // namespace raphEngine::terrain
