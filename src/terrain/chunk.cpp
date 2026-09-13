#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/chunk.hpp>
#include <RaphEngine2/terrain/chunk_format.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#    include <windows.h>
#else
#    include <fcntl.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    namespace
    {
        constexpr uint32_t kEditOverlayMagic = 0x52544545;
        constexpr uint32_t kEditOverlayVersion = 1;

        constexpr uint32_t kPaintOverlayMagic = 0x52545054;
        constexpr uint32_t kPaintOverlayVersion = 1;

#pragma pack(push, 1)
        struct EditOverlayHeader
        {
            uint32_t magic;
            uint32_t version;
            uint32_t tileCount;
        };

        struct PaintOverlayHeader
        {
            uint32_t magic;
            uint32_t version;
            uint32_t tileCount;
        };
#pragma pack(pop)

        static_assert(sizeof(EditOverlayHeader) == 12,
                      "EditOverlayHeader layout must stay byte-exact for "
                      "on-disk compatibility");
        static_assert(sizeof(PaintOverlayHeader) == 12,
                      "PaintOverlayHeader layout must stay byte-exact for "
                      "on-disk compatibility");

        fs::path DeriveEditsPath(const fs::path& basePath)
        {
            return basePath.parent_path()
                / (basePath.filename().string() + ".edits");
        }

        fs::path DerivePaintPath(const fs::path& basePath)
        {
            return basePath.parent_path()
                / (basePath.filename().string() + ".paint");
        }
    } // namespace

    struct Chunk::MappedFile
    {
        MappedFile() = default;
        ~MappedFile()
        {
            Close();
        }

        MappedFile(const MappedFile&) = delete;
        MappedFile& operator=(const MappedFile&) = delete;

        bool Open(const fs::path& path)
        {
            Close();

#if defined(_WIN32)
            fileHandle = CreateFileW(path.c_str(), GENERIC_READ,
                                     FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                return false;
            }

            LARGE_INTEGER fileSize{};
            if (!GetFileSizeEx(fileHandle, &fileSize))
            {
                Close();
                return false;
            }
            size = static_cast<size_t>(fileSize.QuadPart);

            mappingHandle = CreateFileMappingW(fileHandle, nullptr,
                                               PAGE_READONLY, 0, 0, nullptr);
            if (mappingHandle == nullptr)
            {
                Close();
                return false;
            }

            mapped = MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, size);
            if (mapped == nullptr)
            {
                Close();
                return false;
            }
#else
            fd = ::open(path.c_str(), O_RDONLY);
            if (fd < 0)
            {
                return false;
            }

            struct stat fileStat
            {};
            if (::fstat(fd, &fileStat) != 0)
            {
                Close();
                return false;
            }
            size = static_cast<size_t>(fileStat.st_size);

            mapped = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (mapped == MAP_FAILED)
            {
                mapped = nullptr;
                Close();
                return false;
            }
#endif
            return true;
        }

        void Close()
        {
#if defined(_WIN32)
            if (mapped != nullptr)
            {
                UnmapViewOfFile(mapped);
                mapped = nullptr;
            }
            if (mappingHandle != nullptr)
            {
                CloseHandle(mappingHandle);
                mappingHandle = nullptr;
            }
            if (fileHandle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(fileHandle);
                fileHandle = INVALID_HANDLE_VALUE;
            }
#else
            if (mapped != nullptr)
            {
                ::munmap(mapped, size);
                mapped = nullptr;
            }
            if (fd >= 0)
            {
                ::close(fd);
                fd = -1;
            }
#endif
            size = 0;
        }

        const uint8_t* Data() const
        {
            return static_cast<const uint8_t*>(mapped);
        }
        size_t Size() const
        {
            return size;
        }

        void* mapped = nullptr;
        size_t size = 0;

#if defined(_WIN32)
        HANDLE fileHandle = INVALID_HANDLE_VALUE;
        HANDLE mappingHandle = nullptr;
#else
        int fd = -1;
#endif
    };

    Chunk::Chunk() = default;
    Chunk::~Chunk() = default;
    Chunk::Chunk(Chunk&&) noexcept = default;
    Chunk& Chunk::operator=(Chunk&&) noexcept = default;

    void Chunk::Load(const fs::path& p)
    {
        m_editTiles.clear();
        m_paintTiles.clear();
        m_mergedHeights.clear();
        m_mergedPaintMask.clear();
        m_mipPyramid = {};
        m_dirty = false;
        m_state = ChunkState::Unloaded;

        auto mappedFile = std::make_unique<MappedFile>();
        if (!mappedFile->Open(p))
        {
            throw std::runtime_error("Chunk::Load: failed to open "
                                     + p.string());
        }

        if (mappedFile->Size() < sizeof(detail::ChunkFileHeader))
        {
            throw std::runtime_error(
                "Chunk::Load: file too small to contain a header: "
                + p.string());
        }

        detail::ChunkFileHeader header{};
        std::memcpy(&header, mappedFile->Data(),
                    sizeof(detail::ChunkFileHeader));

        if (header.magic != detail::kChunkFileMagic)
        {
            throw std::runtime_error("Chunk::Load: bad magic number in "
                                     + p.string());
        }

        if (header.version != detail::kChunkFileVersion)
        {
            throw std::runtime_error(
                "Chunk::Load: unsupported chunk file version in " + p.string());
        }

        if (header.resolution != kChunkResolution)
        {
            throw std::runtime_error("Chunk::Load: resolution mismatch in "
                                     + p.string());
        }

        const std::vector<glm::ivec2> mipDims =
            detail::ComputeMipLevelDims(header.resolution);
        if (header.mipLevelCount != mipDims.size())
        {
            throw std::runtime_error("Chunk::Load: mip level count mismatch in "
                                     + p.string());
        }

        size_t expectedSize = sizeof(detail::ChunkFileHeader);
        const size_t heightsBytes = static_cast<size_t>(kChunkResolution)
            * kChunkResolution * sizeof(uint16_t);
        expectedSize += heightsBytes;
        for (const glm::ivec2& dim : mipDims)
        {
            expectedSize += static_cast<size_t>(dim.x)
                * static_cast<size_t>(dim.y) * sizeof(HeightRange);
        }

        if (mappedFile->Size() != expectedSize)
        {
            throw std::runtime_error(
                "Chunk::Load: file size mismatch in " + p.string()
                + " (expected " + std::to_string(expectedSize) + ", got "
                + std::to_string(mappedFile->Size()) + ")");
        }

        const uint8_t* cursor =
            mappedFile->Data() + sizeof(detail::ChunkFileHeader);

        m_baseHeights = std::span<const uint16_t>(
            reinterpret_cast<const uint16_t*>(cursor),
            static_cast<size_t>(kChunkResolution) * kChunkResolution);
        cursor += heightsBytes;

        m_mipPyramid.reserve(mipDims.size());
        for (const glm::ivec2& dim : mipDims)
        {
            const size_t levelCount =
                static_cast<size_t>(dim.x) * static_cast<size_t>(dim.y);
            m_mipPyramid.emplace_back(
                reinterpret_cast<const HeightRange*>(cursor), levelCount);
            cursor += levelCount * sizeof(HeightRange);
        }

        m_gridCoord = { header.gridCoordX, header.gridCoordY };
        m_worldHeightRange = { header.worldHeightMin, header.worldHeightMax };
        m_source = static_cast<ChunkSource>(header.source);
        m_dirtyHeightRange = m_mipPyramid.back()[0];

        const fs::path editsPath = DeriveEditsPath(p);
        if (fs::exists(editsPath))
        {
            std::ifstream editsFile(editsPath, std::ios::binary);
            if (!editsFile)
            {
                throw std::runtime_error(
                    "Chunk::Load: failed to open edits file "
                    + editsPath.string());
            }

            EditOverlayHeader editsHeader{};
            editsFile.read(reinterpret_cast<char*>(&editsHeader),
                           sizeof(editsHeader));
            if (!editsFile || editsHeader.magic != kEditOverlayMagic
                || editsHeader.version != kEditOverlayVersion)
            {
                throw std::runtime_error("Chunk::Load: malformed edits file "
                                         + editsPath.string());
            }

            if (editsHeader.tileCount > kEditTilesPerAxis * kEditTilesPerAxis)
            {
                throw std::runtime_error(
                    "Chunk::Load: implausible edit tile count in "
                    + editsPath.string());
            }

            for (uint32_t i = 0; i < editsHeader.tileCount; ++i)
            {
                uint32_t tileId = 0;
                editsFile.read(reinterpret_cast<char*>(&tileId),
                               sizeof(tileId));

                if (tileId >= kEditTilesPerAxis * kEditTilesPerAxis)
                {
                    throw std::runtime_error(
                        "Chunk::Load: invalid edit tile id in "
                        + editsPath.string());
                }

                EditTile tile{};
                editsFile.read(reinterpret_cast<char*>(tile.heights.data()),
                               tile.heights.size() * sizeof(uint16_t));

                if (!editsFile)
                {
                    throw std::runtime_error(
                        "Chunk::Load: truncated edits file "
                        + editsPath.string());
                }

                m_editTiles.emplace(tileId, tile);
            }
        }

        const fs::path paintPath = DerivePaintPath(p);
        if (fs::exists(paintPath))
        {
            std::ifstream paintFile(paintPath, std::ios::binary);
            if (!paintFile)
            {
                throw std::runtime_error(
                    "Chunk::Load: failed to open paint file "
                    + paintPath.string());
            }

            PaintOverlayHeader paintHeader{};
            paintFile.read(reinterpret_cast<char*>(&paintHeader),
                           sizeof(paintHeader));
            if (!paintFile || paintHeader.magic != kPaintOverlayMagic
                || paintHeader.version != kPaintOverlayVersion)
            {
                throw std::runtime_error("Chunk::Load: malformed paint file "
                                         + paintPath.string());
            }

            if (paintHeader.tileCount > kEditTilesPerAxis * kEditTilesPerAxis)
            {
                throw std::runtime_error(
                    "Chunk::Load: implausible paint tile count in "
                    + paintPath.string());
            }

            for (uint32_t i = 0; i < paintHeader.tileCount; ++i)
            {
                uint32_t tileId = 0;
                paintFile.read(reinterpret_cast<char*>(&tileId),
                               sizeof(tileId));

                if (tileId >= kEditTilesPerAxis * kEditTilesPerAxis)
                {
                    throw std::runtime_error(
                        "Chunk::Load: invalid paint tile id in "
                        + paintPath.string());
                }

                PaintTile tile{};
                paintFile.read(
                    reinterpret_cast<char*>(tile.materialIndices.data()),
                    tile.materialIndices.size());

                if (!paintFile)
                {
                    throw std::runtime_error(
                        "Chunk::Load: truncated paint file "
                        + paintPath.string());
                }

                m_paintTiles.emplace(tileId, tile);
            }
        }

        m_mappedFile = std::move(mappedFile);
        m_state = ChunkState::Resident;
        Touch();
    }

    void Chunk::Save(const fs::path& p)
    {
        const fs::path editsPath = DeriveEditsPath(p);

        if (m_editTiles.empty())
        {
            if (fs::exists(editsPath))
            {
                fs::remove(editsPath);
            }
        }
        else
        {
            std::ofstream editsFile(editsPath,
                                    std::ios::binary | std::ios::trunc);
            if (!editsFile)
            {
                throw std::runtime_error("Chunk::Save: failed to open "
                                         + editsPath.string() + " for writing");
            }

            EditOverlayHeader header{};
            header.magic = kEditOverlayMagic;
            header.version = kEditOverlayVersion;
            header.tileCount = static_cast<uint32_t>(m_editTiles.size());
            editsFile.write(reinterpret_cast<const char*>(&header),
                            sizeof(header));

            for (const auto& [tileId, tile] : m_editTiles)
            {
                editsFile.write(reinterpret_cast<const char*>(&tileId),
                                sizeof(tileId));
                editsFile.write(
                    reinterpret_cast<const char*>(tile.heights.data()),
                    tile.heights.size() * sizeof(uint16_t));
            }

            if (!editsFile)
            {
                throw std::runtime_error("Chunk::Save: write failure for "
                                         + editsPath.string());
            }
        }

        const fs::path paintPath = DerivePaintPath(p);

        if (m_paintTiles.empty())
        {
            if (fs::exists(paintPath))
            {
                fs::remove(paintPath);
            }
        }
        else
        {
            std::ofstream paintFile(paintPath,
                                    std::ios::binary | std::ios::trunc);
            if (!paintFile)
            {
                throw std::runtime_error("Chunk::Save: failed to open "
                                         + paintPath.string() + " for writing");
            }

            PaintOverlayHeader header{};
            header.magic = kPaintOverlayMagic;
            header.version = kPaintOverlayVersion;
            header.tileCount = static_cast<uint32_t>(m_paintTiles.size());
            paintFile.write(reinterpret_cast<const char*>(&header),
                            sizeof(header));

            for (const auto& [tileId, tile] : m_paintTiles)
            {
                paintFile.write(reinterpret_cast<const char*>(&tileId),
                                sizeof(tileId));
                paintFile.write(
                    reinterpret_cast<const char*>(tile.materialIndices.data()),
                    tile.materialIndices.size());
            }

            if (!paintFile)
            {
                throw std::runtime_error("Chunk::Save: write failure for "
                                         + paintPath.string());
            }
        }

        m_dirty = false;
    }

    void Chunk::Unload()
    {
        m_mappedFile.reset();
        m_baseHeights = {};
        m_mipPyramid.clear();
        m_mergedHeights.clear();
        m_mergedPaintMask.clear();
        m_state = ChunkState::Unloaded;
    }

    uint16_t Chunk::SampleRawHeight(glm::ivec2 texel) const
    {
        texel = glm::clamp(texel, glm::ivec2(0),
                           glm::ivec2(static_cast<int>(kChunkResolution) - 1));

        const uint32_t tileId = ComputeTileId(texel);
        const auto it = m_editTiles.find(tileId);
        if (it != m_editTiles.end())
        {
            const int localX = texel.x % static_cast<int>(kEditTileResolution);
            const int localY = texel.y % static_cast<int>(kEditTileResolution);
            return it->second.heights[localY * kEditTileResolution + localX];
        }

        return m_baseHeights[texel.y * kChunkResolution + texel.x];
    }

    uint32_t Chunk::ComputeTileId(glm::ivec2 texel) const
    {
        const uint32_t tileX =
            static_cast<uint32_t>(texel.x) / kEditTileResolution;
        const uint32_t tileY =
            static_cast<uint32_t>(texel.y) / kEditTileResolution;
        return tileY * kEditTilesPerAxis + tileX;
    }

    float Chunk::ToWorldHeight(uint16_t rawHeight) const
    {
        const float t = static_cast<float>(rawHeight) / 65535.0f;
        return glm::mix(m_worldHeightRange.x, m_worldHeightRange.y, t);
    }

    uint16_t Chunk::FromWorldHeight(float worldHeight) const
    {
        const float range = m_worldHeightRange.y - m_worldHeightRange.x;
        if (range <= 0.0f)
        {
            return 0;
        }

        const float t = glm::clamp((worldHeight - m_worldHeightRange.x) / range,
                                   0.0f, 1.0f);
        return static_cast<uint16_t>(t * 65535.0f + 0.5f);
    }

    float Chunk::GetHeightAt(glm::vec2 localPosition) const
    {
        const glm::vec2 maxCoord(static_cast<float>(kChunkResolution - 1));
        const glm::vec2 clamped =
            glm::clamp(localPosition, glm::vec2(0.0f), maxCoord);

        const glm::ivec2 texel0(clamped);
        const glm::ivec2 texel1 =
            glm::min(texel0 + glm::ivec2(1, 1),
                     glm::ivec2(static_cast<int>(kChunkResolution) - 1));
        const glm::vec2 frac = clamped - glm::vec2(texel0);

        const float h00 =
            ToWorldHeight(SampleRawHeight({ texel0.x, texel0.y }));
        const float h10 =
            ToWorldHeight(SampleRawHeight({ texel1.x, texel0.y }));
        const float h01 =
            ToWorldHeight(SampleRawHeight({ texel0.x, texel1.y }));
        const float h11 =
            ToWorldHeight(SampleRawHeight({ texel1.x, texel1.y }));

        const float top = glm::mix(h00, h10, frac.x);
        const float bottom = glm::mix(h01, h11, frac.x);
        return glm::mix(top, bottom, frac.y);
    }

    HeightRange Chunk::GetChunkHeightRange() const
    {
        return m_dirtyHeightRange;
    }

    void Chunk::PaintHeight(glm::ivec2 texel, float worldHeight)
    {
        texel = glm::clamp(texel, glm::ivec2(0),
                           glm::ivec2(static_cast<int>(kChunkResolution) - 1));

        const uint32_t tileId = ComputeTileId(texel);
        auto it = m_editTiles.find(tileId);
        if (it == m_editTiles.end())
        {
            EditTile newTile{};
            const glm::ivec2 tileOrigin(
                static_cast<int>(tileId % kEditTilesPerAxis)
                    * static_cast<int>(kEditTileResolution),
                static_cast<int>(tileId / kEditTilesPerAxis)
                    * static_cast<int>(kEditTileResolution));

            for (uint32_t y = 0; y < kEditTileResolution; ++y)
            {
                for (uint32_t x = 0; x < kEditTileResolution; ++x)
                {
                    const glm::ivec2 sourceTexel(
                        tileOrigin.x + static_cast<int>(x),
                        tileOrigin.y + static_cast<int>(y));
                    newTile.heights[y * kEditTileResolution + x] =
                        SampleRawHeight(sourceTexel);
                }
            }

            it = m_editTiles.emplace(tileId, newTile).first;
        }

        const uint16_t rawHeight = FromWorldHeight(worldHeight);
        const int localX = texel.x % static_cast<int>(kEditTileResolution);
        const int localY = texel.y % static_cast<int>(kEditTileResolution);
        it->second.heights[localY * kEditTileResolution + localX] = rawHeight;

        m_dirtyHeightRange.min = std::min(m_dirtyHeightRange.min, rawHeight);
        m_dirtyHeightRange.max = std::max(m_dirtyHeightRange.max, rawHeight);

        m_mergedHeights.clear();
        m_dirty = true;
        ++m_gpuDataVersion;
    }

    std::span<const uint16_t> Chunk::GetHeightDataForUpload() const
    {
        if (m_editTiles.empty())
        {
            return m_baseHeights;
        }

        if (m_mergedHeights.empty())
        {
            m_mergedHeights.assign(m_baseHeights.begin(), m_baseHeights.end());

            for (const auto& [tileId, tile] : m_editTiles)
            {
                const glm::ivec2 tileOrigin(
                    static_cast<int>(tileId % kEditTilesPerAxis)
                        * static_cast<int>(kEditTileResolution),
                    static_cast<int>(tileId / kEditTilesPerAxis)
                        * static_cast<int>(kEditTileResolution));

                for (uint32_t y = 0; y < kEditTileResolution; ++y)
                {
                    for (uint32_t x = 0; x < kEditTileResolution; ++x)
                    {
                        const size_t destIndex =
                            static_cast<size_t>(tileOrigin.y + y)
                                * kChunkResolution
                            + static_cast<size_t>(tileOrigin.x + x);
                        m_mergedHeights[destIndex] =
                            tile.heights[y * kEditTileResolution + x];
                    }
                }
            }
        }

        return m_mergedHeights;
    }

    void Chunk::PaintTexture(glm::ivec2 texel, uint8_t materialIndex)
    {
        texel = glm::clamp(texel, glm::ivec2(0),
                           glm::ivec2(static_cast<int>(kChunkResolution) - 1));

        const uint32_t tileId = ComputeTileId(texel);
        auto it = m_paintTiles.find(tileId);
        if (it == m_paintTiles.end())
        {
            it = m_paintTiles.emplace(tileId, PaintTile{}).first;
        }

        const int localX = texel.x % static_cast<int>(kEditTileResolution);
        const int localY = texel.y % static_cast<int>(kEditTileResolution);
        it->second.materialIndices[localY * kEditTileResolution + localX] =
            materialIndex;

        m_mergedPaintMask.clear();
        m_dirty = true;
        ++m_gpuDataVersion;
    }

    std::span<const uint8_t> Chunk::GetPaintDataForUpload() const
    {
        if (m_mergedPaintMask.empty())
        {
            m_mergedPaintMask.assign(
                static_cast<size_t>(kChunkResolution) * kChunkResolution, 0);

            for (const auto& [tileId, tile] : m_paintTiles)
            {
                const glm::ivec2 tileOrigin(
                    static_cast<int>(tileId % kEditTilesPerAxis)
                        * static_cast<int>(kEditTileResolution),
                    static_cast<int>(tileId / kEditTilesPerAxis)
                        * static_cast<int>(kEditTileResolution));

                for (uint32_t y = 0; y < kEditTileResolution; ++y)
                {
                    for (uint32_t x = 0; x < kEditTileResolution; ++x)
                    {
                        const size_t destIndex =
                            static_cast<size_t>(tileOrigin.y + y)
                                * kChunkResolution
                            + static_cast<size_t>(tileOrigin.x + x);
                        m_mergedPaintMask[destIndex] =
                            tile.materialIndices[y * kEditTileResolution + x];
                    }
                }
            }
        }

        return m_mergedPaintMask;
    }

    void Chunk::Touch()
    {
        m_lastAccessTime = std::chrono::steady_clock::now();
    }
} // namespace raphEngine::terrain
