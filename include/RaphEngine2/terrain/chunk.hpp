#pragma once

#include <RaphEngine2/RaphEngine2.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    inline constexpr uint32_t kChunkResolution = 250;
    inline constexpr uint32_t kEditTileResolution = 50;
    inline constexpr uint32_t kEditTilesPerAxis =
        kChunkResolution / kEditTileResolution;

    struct HeightRange
    {
        uint16_t min;
        uint16_t max;
    };

    struct EditTile
    {
        std::array<uint16_t, kEditTileResolution * kEditTileResolution> heights;
    };

    struct PaintTile
    {
        std::array<uint8_t, kEditTileResolution * kEditTileResolution>
            materialIndices;
    };

    enum class ChunkSource : uint8_t
    {
        Procedural,
        Imported
    };

    enum class ChunkState : uint8_t
    {
        Unloaded,
        Resident
    };

    class RAPHENGINE_API Chunk
    {
    public:
        Chunk();
        ~Chunk();

        Chunk(const Chunk&) = delete;
        Chunk& operator=(const Chunk&) = delete;
        Chunk(Chunk&&) noexcept;
        Chunk& operator=(Chunk&&) noexcept;

        /// Memory-maps the baked chunk file at `path` and, if present, loads
        /// sibling sparse .edits and .paint overlay files. Throws on I/O or
        /// format failure.
        void Load(const fs::path&);

        /// Writes the sparse .edits and .paint overlays; a chunk with
        /// neither writes nothing.
        void Save(const fs::path&);

        void Unload();

        float GetHeightAt(glm::vec2 localPosition) const;
        HeightRange GetChunkHeightRange() const;
        glm::vec2 GetWorldHeightRange() const noexcept
        {
            return m_worldHeightRange;
        }

        void PaintHeight(glm::ivec2 texel, float worldHeight);
        std::span<const uint16_t> GetHeightDataForUpload() const;

        /// materialIndex 0 means "use automatic height/slope selection"; any
        /// other value N selects palette layer (N - 1) in the renderer's
        /// material array, overriding the automatic blend at that texel.
        void PaintTexture(glm::ivec2 texel, uint8_t materialIndex);
        std::span<const uint8_t> GetPaintDataForUpload() const;

        glm::ivec2 GetGridCoord() const noexcept
        {
            return m_gridCoord;
        }
        void SetGridCoord(glm::ivec2 coord) noexcept
        {
            m_gridCoord = coord;
        }

        ChunkSource GetSource() const noexcept
        {
            return m_source;
        }
        ChunkState GetState() const noexcept
        {
            return m_state;
        }
        bool IsDirty() const noexcept
        {
            return m_dirty;
        }

        /// Increments on any edit (PaintHeight/PaintTexture) that changes
        /// GPU-visible data. A renderer can compare this against the value
        /// it last uploaded to decide whether a resident chunk needs
        /// re-uploading, without re-uploading every chunk every frame.
        uint64_t GetGpuDataVersion() const noexcept
        {
            return m_gpuDataVersion;
        }

        void Touch();
        std::chrono::steady_clock::time_point GetLastAccessTime() const noexcept
        {
            return m_lastAccessTime;
        }

    private:
        uint16_t SampleRawHeight(glm::ivec2 texel) const;
        uint32_t ComputeTileId(glm::ivec2 texel) const;

        float ToWorldHeight(uint16_t rawHeight) const;
        uint16_t FromWorldHeight(float worldHeight) const;

        struct MappedFile;
        std::unique_ptr<MappedFile> m_mappedFile;

        std::span<const uint16_t> m_baseHeights;
        std::vector<std::span<const HeightRange>> m_mipPyramid;
        std::unordered_map<uint32_t, EditTile> m_editTiles;
        std::unordered_map<uint32_t, PaintTile> m_paintTiles;

        mutable std::vector<uint16_t> m_mergedHeights;
        mutable std::vector<uint8_t> m_mergedPaintMask;

        glm::ivec2 m_gridCoord{ 0, 0 };
        glm::vec2 m_worldHeightRange{ 0.0f, 0.0f };
        HeightRange m_dirtyHeightRange{ 0, 0 };

        ChunkSource m_source = ChunkSource::Procedural;
        ChunkState m_state = ChunkState::Unloaded;
        bool m_dirty = false;
        uint64_t m_gpuDataVersion = 0;

        std::chrono::steady_clock::time_point m_lastAccessTime{};
    };
} // namespace raphEngine::terrain
