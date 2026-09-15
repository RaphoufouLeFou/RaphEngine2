#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/graphics/terrain_renderer.hpp>

#include <cstdint>
#include <memory>
#include <span>
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
        struct RingState
        {
            glm::vec2 snappedOrigin{ 0.0f, 0.0f };
            bool initialized = false;
        };

        void CreateRingMeshes();
        void CreateHeightRingArray();
        void CreateNormalRingArray();
        void CreateMaterialTextureArrays();
        void CreateGaussianAlbedoArrays();
        unsigned int
        CreateMaterialMapArray(std::span<const char* const> paths) const;

        float GetRingWorldSize(uint32_t ringIndex) const;
        glm::vec2 ComputeSnappedOrigin(uint32_t ringIndex,
                                       glm::vec2 cameraXY) const;
        void UpdateRingTexture(uint32_t ringIndex, const terrain::Map& map,
                               glm::vec2 snappedOrigin);
        void DrawRing(uint32_t ringIndex, const Shader* shader) const;

        static constexpr uint32_t kRingCount = 12;
        static constexpr uint32_t kRingResolution = 128;
        static constexpr float kBaseRingWorldSize = 64.0f;

        static constexpr float kRockPatchScale = 60.0f;
        static constexpr float kDirtPatchScale = 45.0f;
        static constexpr float kMinNormalSampleDistance = 2.0f;

        static constexpr float kFogDensity = 0.00006f;
        static constexpr glm::vec3 kFogColor = glm::vec3(0.55f, 0.62f, 0.70f);

        unsigned int ringVertexBuffer_ = 0;
        unsigned int solidVao_ = 0;
        unsigned int solidEbo_ = 0;
        uint32_t solidIndexCount_ = 0;

        unsigned int heightRingArray_ = 0;
        unsigned int normalRingArray_ = 0;

        unsigned int materialGaussianAlbedoArray_ = 0;
        unsigned int materialAlbedoLutArray_ = 0;
        unsigned int materialNormalArray_ = 0;
        unsigned int materialOrmArray_ = 0;

        std::vector<RingState> ringStates_;

        std::vector<float> scratchHeights_;
        std::vector<float> scratchNormalGridHeights_;
        std::vector<glm::vec3> scratchNormalGridNormals_;
        std::vector<glm::vec4> scratchRingData_;
        std::vector<glm::vec3> scratchNormalData_;

        std::shared_ptr<Shader> terrainShader_;
    };
} // namespace raphEngine::graphics::ogl
