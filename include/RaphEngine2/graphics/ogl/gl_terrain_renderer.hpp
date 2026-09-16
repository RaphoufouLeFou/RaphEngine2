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
}

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
        struct QuadNode
        {
            int level;
            int x;
            int y;
            glm::vec2 origin;
            float size;
        };

        struct QuadCandidate
        {
            QuadNode node;
            float distance;
        };

        struct QuadCandidateCompare
        {
            bool operator()(const QuadCandidate& a,
                            const QuadCandidate& b) const
            {
                return a.distance > b.distance;
            }
        };

        struct NodeKey
        {
            int level;
            int x;
            int y;

            bool operator==(const NodeKey&) const = default;
        };

        struct NodeKeyHash
        {
            size_t operator()(const NodeKey& k) const
            {
                size_t h = std::hash<int>{}(k.level);
                h ^= std::hash<int>{}(k.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= std::hash<int>{}(k.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
                return h;
            }
        };

        struct ResidentNode
        {
            uint32_t layer;
            bool inUseThisFrame;
        };

        void CreateNodeMesh();
        void CreateHeightNodeArray();
        void CreateNormalNodeArray();
        void CreateMaterialTextureArrays();
        void CreateGaussianAlbedoArrays();
        unsigned int
        CreateMaterialMapArray(std::span<const char* const> paths) const;

        QuadCandidate MakeCandidate(int level, int x, int y,
                                    glm::vec2 cameraXY) const;
        void BuildLeafSet(glm::vec2 cameraXY, glm::vec2 worldMin,
                          glm::vec2 worldMax,
                          std::vector<QuadNode>& outLeaves) const;

        void RasterizeLevelGrid(const std::vector<QuadNode>& leaves,
                                int64_t& outMinX, int64_t& outMinY,
                                int64_t& outWidth, int64_t& outHeight) const;
        void BalanceLeafSet(std::vector<QuadNode>& leaves) const;
        void InvalidateAllNodes();

        void BuildNodeData(const QuadNode& node, const terrain::Map& map,
                           uint32_t layer);
        void DrawNode(const QuadNode& node, uint32_t layer,
                      const Shader* shader) const;

        static constexpr uint32_t kNodeResolution = 32;
        static constexpr float kLeafWorldSize = 64.0f;
        static constexpr int kMaxLevel = 8;
        static constexpr float kSplitDistanceFactor = 1.5f;
        static constexpr uint32_t kMaxActiveNodes = 2048;

        static constexpr float kRockPatchScale = 60.0f;
        static constexpr float kDirtPatchScale = 45.0f;
        static constexpr float kFixedNormalSampleDistance = 2.0f;

        unsigned int nodeVertexBuffer_ = 0;
        unsigned int nodeVao_ = 0;
        unsigned int nodeEbo_ = 0;
        uint32_t nodeIndexCount_ = 0;

        unsigned int heightNodeArray_ = 0;
        unsigned int normalNodeArray_ = 0;

        unsigned int materialGaussianAlbedoArray_ = 0;
        unsigned int materialAlbedoLutArray_ = 0;
        unsigned int materialNormalArray_ = 0;
        unsigned int materialOrmArray_ = 0;

        std::unordered_map<NodeKey, ResidentNode, NodeKeyHash> residentNodes_;
        std::vector<bool> layerInUse_;
        mutable std::vector<int16_t> scratchLevelGrid_;

        uint64_t lastSeenMapGeneration_ = 0;

        std::shared_ptr<Shader> terrainShader_;
    };
} // namespace raphEngine::graphics::ogl
