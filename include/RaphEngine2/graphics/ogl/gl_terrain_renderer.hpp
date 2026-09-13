#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/graphics/terrain_renderer.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

namespace raphEngine::graphics
{
    class Shader;
}

namespace raphEngine::terrain
{
    class Map;
    class Chunk;
} // namespace raphEngine::terrain

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API GLTerrainRenderer final : public TerrainRenderer
    {
    public:
        GLTerrainRenderer();
        ~GLTerrainRenderer() override;

        GLTerrainRenderer(const GLTerrainRenderer&) = delete;
        GLTerrainRenderer& operator=(const GLTerrainRenderer&) = delete;

        void render(const terrain::Map& map) override;

    private:
        struct ChunkGpuSlot
        {
            uint32_t textureLayer;
            uint64_t uploadedVersion;
            bool inUseThisFrame;
        };

        struct LayerAcquireResult
        {
            uint32_t layer;
            bool needsUpload;
        };

        void CreatePatchMesh();
        void CreateHeightTextureArray();
        void CreatePaintMaskTextureArray();
        void CreateMaterialTextureArrays();
        void CreateGaussianAlbedoArrays();
        void CreateInstanceBuffer();

        unsigned int
        CreateMaterialMapArray(std::span<const char* const> paths) const;

        LayerAcquireResult AcquireTextureLayer(glm::ivec2 gridCoord,
                                               uint64_t currentVersion);
        void ReleaseUnusedLayers();
        void UploadChunkHeights(const terrain::Chunk& chunk, uint32_t layer);
        void UploadChunkPaintMask(const terrain::Chunk& chunk, uint32_t layer);

        static constexpr uint32_t kMaxResidentChunks = 512;
        static constexpr uint32_t kPatchGridSize = 8;

        unsigned int patchVao_ = 0;
        unsigned int patchVbo_ = 0;
        unsigned int patchEbo_ = 0;
        uint32_t patchCount_ = 0;

        unsigned int heightTextureArray_ = 0;
        unsigned int paintMaskTextureArray_ = 0;
        unsigned int materialGaussianAlbedoArray_ = 0;
        unsigned int materialAlbedoLutArray_ = 0;
        unsigned int materialNormalArray_ = 0;
        unsigned int materialOrmArray_ = 0;
        unsigned int chunkInstanceSsbo_ = 0;
        void* chunkInstanceSsboPtr_ = nullptr;

        std::unordered_map<uint64_t, ChunkGpuSlot> residentChunks_;
        std::vector<bool> layerInUse_;

        std::shared_ptr<Shader> terrainShader_;
    };
} // namespace raphEngine::graphics::ogl
