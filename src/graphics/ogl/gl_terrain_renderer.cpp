#include "graphics/ogl/gl_terrain_renderer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <RaphEngine2/component/camera_component.hpp>
#include <RaphEngine2/default_shaders.hpp>
#include <RaphEngine2/graphics/ogl/gl_shader.hpp>
#include <RaphEngine2/graphics/shader.hpp>
#include <RaphEngine2/graphics/shadow_renderer.hpp>
#include <RaphEngine2/graphics/skybox.hpp>
#include <RaphEngine2/graphics/stochastic_texture_baker.hpp>
#include <RaphEngine2/graphics/texture_loader.hpp>
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/terrain/map.hpp>
#include <RaphEngine2/utils.hpp>

#include "graphics/ogl/gl_shadow_renderer.hpp"
#include "graphics/ogl/gl_skybox.hpp"

namespace raphEngine::graphics::ogl
{
    namespace
    {
        struct TerrainMaterialDefinition
        {
            const char* albedoPath;
            const char* normalPath;
            const char* ormPath; // R = AO, G = roughness, B = metallic
            float tileSizeMeters;
        };

        constexpr int kAlbedoLutResolution = 256;

        // index 0=grass, 1=rock, 2=snow, 3=dirt — must match the kMaterial*
        // constants in terrain_ring_fs.glsl.
        constexpr std::array<TerrainMaterialDefinition, 4> kMaterials = { {
            { "assets/textures/terrain/forrest_ground_01_diff_4k.jpg",
              "assets/textures/terrain/forrest_ground_01_nor_gl_4k.jpg",
              "assets/textures/terrain/forrest_ground_01_arm_4k.jpg", 2.0f },
            { "assets/textures/terrain/rock_face_03_diff_4k.jpg",
              "assets/textures/terrain/rock_face_03_nor_gl_4k.jpg",
              "assets/textures/terrain/rock_face_03_arm_4k.jpg", 2.0f },
            { "assets/textures/terrain/snow_02_diff_4k.jpg",
              "assets/textures/terrain/snow_02_nor_gl_4k.jpg",
              "assets/textures/terrain/snow_02_arm_4k.jpg", 2.0f },
            { "assets/textures/terrain/forest_ground_05_diff_4k.jpg",
              "assets/textures/terrain/forest_ground_05_nor_gl_4k.jpg",
              "assets/textures/terrain/forest_ground_05_arm_4k.jpg", 2.0f },
        } };

        float MacroHash(glm::vec2 p)
        {
            p = 50.0f * glm::fract(p * 0.3183099f + glm::vec2(0.71f, 0.113f));
            return glm::fract(p.x * p.y * (p.x + p.y));
        }

        float MacroNoise(glm::vec2 p)
        {
            glm::vec2 i = glm::floor(p);
            glm::vec2 f = glm::fract(p);
            glm::vec2 u = f * f * (3.0f - 2.0f * f);

            float a = MacroHash(i);
            float b = MacroHash(i + glm::vec2(1.0f, 0.0f));
            float c = MacroHash(i + glm::vec2(0.0f, 1.0f));
            float d = MacroHash(i + glm::vec2(1.0f, 1.0f));

            return glm::mix(glm::mix(a, b, u.x), glm::mix(c, d, u.x), u.y);
        }
    } // namespace

    GLTerrainRenderer::GLTerrainRenderer()
    {
        CreateRingMeshes();
        CreateHeightRingArray();
        CreateNormalRingArray();
        CreateMaterialTextureArrays();

        terrainShader_ = Shader::loadShader(
            ShaderStages{ .vertex = terrain_ring_vs_shader,
                          .fragment = terrain_ring_fs_shader });

        terrainShader_->bindUniformBlock("LightSpaceMatrices", 0);
    }

    GLTerrainRenderer::~GLTerrainRenderer()
    {
        glDeleteTextures(1, &materialOrmArray_);
        glDeleteTextures(1, &materialNormalArray_);
        glDeleteTextures(1, &materialAlbedoLutArray_);
        glDeleteTextures(1, &materialGaussianAlbedoArray_);
        glDeleteTextures(1, &normalRingArray_);
        glDeleteTextures(1, &heightRingArray_);
        glDeleteBuffers(1, &solidEbo_);
        glDeleteVertexArrays(1, &solidVao_);
        glDeleteBuffers(1, &ringVertexBuffer_);
    }

    void GLTerrainRenderer::CreateRingMeshes()
    {
        std::vector<glm::vec2> vertices;
        vertices.reserve((kRingResolution + 1) * (kRingResolution + 1));

        for (uint32_t y = 0; y <= kRingResolution; ++y)
        {
            for (uint32_t x = 0; x <= kRingResolution; ++x)
            {
                vertices.emplace_back(static_cast<float>(x) / kRingResolution,
                                      static_cast<float>(y) / kRingResolution);
            }
        }

        const uint32_t rowStride = kRingResolution + 1;
        std::vector<uint32_t> indices;
        indices.reserve(static_cast<size_t>(kRingResolution) * kRingResolution
                        * 6);

        for (uint32_t y = 0; y < kRingResolution; ++y)
        {
            for (uint32_t x = 0; x < kRingResolution; ++x)
            {
                const uint32_t i0 = y * rowStride + x;
                const uint32_t i1 = i0 + 1;
                const uint32_t i2 = i0 + rowStride;
                const uint32_t i3 = i2 + 1;

                indices.insert(indices.end(), { i0, i1, i2, i1, i3, i2 });
            }
        }

        solidIndexCount_ = static_cast<uint32_t>(indices.size());

        glCreateBuffers(1, &ringVertexBuffer_);
        glNamedBufferStorage(
            ringVertexBuffer_,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec2)),
            vertices.data(), 0);

        glCreateVertexArrays(1, &solidVao_);
        glCreateBuffers(1, &solidEbo_);
        glNamedBufferStorage(
            solidEbo_,
            static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
            indices.data(), 0);
        glVertexArrayVertexBuffer(solidVao_, 0, ringVertexBuffer_, 0,
                                  sizeof(glm::vec2));
        glVertexArrayElementBuffer(solidVao_, solidEbo_);
        glEnableVertexArrayAttrib(solidVao_, 0);
        glVertexArrayAttribFormat(solidVao_, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(solidVao_, 0, 0);
    }

    void GLTerrainRenderer::CreateHeightRingArray()
    {
        const GLsizei texSize = static_cast<GLsizei>(kRingResolution + 1);

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &heightRingArray_);
        glTextureStorage3D(heightRingArray_, 1, GL_RGBA32F, texSize, texSize,
                           static_cast<GLsizei>(kRingCount));

        glTextureParameteri(heightRingArray_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(heightRingArray_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(heightRingArray_, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
        glTextureParameteri(heightRingArray_, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);

        ringStates_.assign(kRingCount, RingState{});
    }

    void GLTerrainRenderer::CreateNormalRingArray()
    {
        const GLsizei texSize = static_cast<GLsizei>(kRingResolution + 1);

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &normalRingArray_);
        glTextureStorage3D(normalRingArray_, 1, GL_RGB16F, texSize, texSize,
                           static_cast<GLsizei>(kRingCount));

        glTextureParameteri(normalRingArray_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(normalRingArray_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(normalRingArray_, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
        glTextureParameteri(normalRingArray_, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);
    }

    unsigned int GLTerrainRenderer::CreateMaterialMapArray(
        std::span<const char* const> paths) const
    {
        graphics::TextureLoader* loader =
            graphics::TextureLoader::getInstance();

        int arrayWidth = 0;
        int arrayHeight = 0;
        int arrayChannels = 0;

        std::vector<graphics::TextureLoader::RawTexture> rawTextures;
        rawTextures.reserve(paths.size());

        for (const char* path : paths)
        {
            graphics::TextureLoader::RawTexture raw =
                loader->load_texture_raw(path);

            if (raw.data == nullptr)
            {
                for (auto& r : rawTextures)
                    loader->free_raw(r);
                throw std::runtime_error(
                    std::string(
                        "GLTerrainRenderer: failed to load terrain texture ")
                    + path);
            }

            if (rawTextures.empty())
            {
                arrayWidth = raw.width;
                arrayHeight = raw.height;
                arrayChannels = raw.nrChannels;
            }
            else if (raw.width != arrayWidth || raw.height != arrayHeight
                     || raw.nrChannels != arrayChannels)
            {
                loader->free_raw(raw);
                for (auto& r : rawTextures)
                    loader->free_raw(r);
                throw std::runtime_error(
                    "GLTerrainRenderer: all textures in one material map array "
                    "must share resolution and channel count ("
                    + std::string(path) + " does not match the others)");
            }

            rawTextures.push_back(raw);
        }

        const GLenum internalFormat = (arrayChannels == 4) ? GL_RGBA8 : GL_RGB8;
        const GLenum uploadFormat = (arrayChannels == 4) ? GL_RGBA : GL_RGB;
        const GLsizei mipLevels =
            static_cast<GLsizei>(std::floor(std::log2(
                static_cast<float>(std::max(arrayWidth, arrayHeight)))))
            + 1;

        unsigned int textureId = 0;
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &textureId);
        glTextureStorage3D(textureId, mipLevels, internalFormat, arrayWidth,
                           arrayHeight,
                           static_cast<GLsizei>(rawTextures.size()));

        for (size_t i = 0; i < rawTextures.size(); ++i)
        {
            glTextureSubImage3D(textureId, 0, 0, 0, static_cast<GLint>(i),
                                arrayWidth, arrayHeight, 1, uploadFormat,
                                GL_UNSIGNED_BYTE, rawTextures[i].data);
            loader->free_raw(rawTextures[i]);
        }

        glGenerateTextureMipmap(textureId);

        glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureId, GL_TEXTURE_WRAP_T, GL_REPEAT);

        return textureId;
    }

    void GLTerrainRenderer::CreateGaussianAlbedoArrays()
    {
        graphics::TextureLoader* loader =
            graphics::TextureLoader::getInstance();

        int arrayWidth = 0;
        int arrayHeight = 0;
        int arrayChannels = 0;

        std::vector<graphics::StochasticTextureData> stochasticData;
        stochasticData.reserve(kMaterials.size());

        for (const TerrainMaterialDefinition& material : kMaterials)
        {
            graphics::TextureLoader::RawTexture raw =
                loader->load_texture_raw(material.albedoPath);
            if (raw.data == nullptr)
            {
                throw std::runtime_error(
                    std::string("GLTerrainRenderer: failed to load terrain "
                                "albedo texture ")
                    + material.albedoPath);
            }

            if (stochasticData.empty())
            {
                arrayWidth = raw.width;
                arrayHeight = raw.height;
                arrayChannels = raw.nrChannels;
            }
            else if (raw.width != arrayWidth || raw.height != arrayHeight
                     || raw.nrChannels != arrayChannels)
            {
                loader->free_raw(raw);
                throw std::runtime_error(
                    "GLTerrainRenderer: all terrain albedo textures must share "
                    "resolution and channel count");
            }

            stochasticData.push_back(graphics::ComputeStochasticTextureData(
                raw.data, raw.width, raw.height, raw.nrChannels,
                kAlbedoLutResolution));
            loader->free_raw(raw);
        }

        const GLenum internalFormat = (arrayChannels == 4) ? GL_RGBA8 : GL_RGB8;
        const GLenum uploadFormat = (arrayChannels == 4) ? GL_RGBA : GL_RGB;
        const GLsizei mipLevels =
            static_cast<GLsizei>(std::floor(std::log2(
                static_cast<float>(std::max(arrayWidth, arrayHeight)))))
            + 1;

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &materialGaussianAlbedoArray_);
        glTextureStorage3D(materialGaussianAlbedoArray_, mipLevels,
                           internalFormat, arrayWidth, arrayHeight,
                           static_cast<GLsizei>(stochasticData.size()));

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &materialAlbedoLutArray_);
        glTextureStorage3D(materialAlbedoLutArray_, 1, internalFormat,
                           kAlbedoLutResolution, 1,
                           static_cast<GLsizei>(stochasticData.size()));

        for (size_t i = 0; i < stochasticData.size(); ++i)
        {
            glTextureSubImage3D(materialGaussianAlbedoArray_, 0, 0, 0,
                                static_cast<GLint>(i), arrayWidth, arrayHeight,
                                1, uploadFormat, GL_UNSIGNED_BYTE,
                                stochasticData[i].gaussianizedPixels.data());

            glTextureSubImage3D(materialAlbedoLutArray_, 0, 0, 0,
                                static_cast<GLint>(i), kAlbedoLutResolution, 1,
                                1, uploadFormat, GL_UNSIGNED_BYTE,
                                stochasticData[i].inverseLut.data());
        }

        glGenerateTextureMipmap(materialGaussianAlbedoArray_);

        glTextureParameteri(materialGaussianAlbedoArray_, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(materialGaussianAlbedoArray_, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);
        glTextureParameteri(materialGaussianAlbedoArray_, GL_TEXTURE_WRAP_S,
                            GL_REPEAT);
        glTextureParameteri(materialGaussianAlbedoArray_, GL_TEXTURE_WRAP_T,
                            GL_REPEAT);

        glTextureParameteri(materialAlbedoLutArray_, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR);
        glTextureParameteri(materialAlbedoLutArray_, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);
        glTextureParameteri(materialAlbedoLutArray_, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
        glTextureParameteri(materialAlbedoLutArray_, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);
    }

    void GLTerrainRenderer::CreateMaterialTextureArrays()
    {
        CreateGaussianAlbedoArrays();

        std::vector<const char*> normalPaths;
        std::vector<const char*> ormPaths;
        normalPaths.reserve(kMaterials.size());
        ormPaths.reserve(kMaterials.size());

        for (const TerrainMaterialDefinition& material : kMaterials)
        {
            normalPaths.push_back(material.normalPath);
            ormPaths.push_back(material.ormPath);
        }

        materialNormalArray_ = CreateMaterialMapArray(normalPaths);
        materialOrmArray_ = CreateMaterialMapArray(ormPaths);
    }

    float GLTerrainRenderer::GetRingWorldSize(uint32_t ringIndex) const
    {
        return kBaseRingWorldSize * static_cast<float>(1u << ringIndex);
    }

    glm::vec2 GLTerrainRenderer::ComputeSnappedOrigin(uint32_t ringIndex,
                                                      glm::vec2 cameraXY) const
    {
        const float worldSize = GetRingWorldSize(ringIndex);
        const float texelSize = worldSize / static_cast<float>(kRingResolution);

        const glm::vec2 idealOrigin = cameraXY - glm::vec2(worldSize * 0.5f);
        return glm::floor(idealOrigin / texelSize) * texelSize;
    }

    void GLTerrainRenderer::UpdateRingTexture(uint32_t ringIndex,
                                              const terrain::Map& map,
                                              glm::vec2 snappedOrigin)
    {
        const uint32_t texSize = kRingResolution + 1;
        const float worldSize = GetRingWorldSize(ringIndex);
        const float texelSize = worldSize / static_cast<float>(kRingResolution);

        scratchHeights_.resize(static_cast<size_t>(texSize) * texSize);
        for (uint32_t y = 0; y < texSize; ++y)
        {
            for (uint32_t x = 0; x < texSize; ++x)
            {
                const glm::vec2 worldXY = snappedOrigin
                    + glm::vec2(static_cast<float>(x), static_cast<float>(y))
                        * texelSize;
                scratchHeights_[y * texSize + x] = map.GetHeightAt(worldXY);
            }
        }

        const float gridStep = std::max(kMinNormalSampleDistance, texelSize);
        const float gridOriginX =
            std::floor((snappedOrigin.x - gridStep) / gridStep) * gridStep;
        const float gridOriginY =
            std::floor((snappedOrigin.y - gridStep) / gridStep) * gridStep;
        const glm::vec2 gridOrigin(gridOriginX, gridOriginY);

        const float gridSpanX =
            (snappedOrigin.x + worldSize + gridStep) - gridOriginX;
        const float gridSpanY =
            (snappedOrigin.y + worldSize + gridStep) - gridOriginY;
        const int gridCountX =
            static_cast<int>(std::ceil(gridSpanX / gridStep)) + 1;
        const int gridCountY =
            static_cast<int>(std::ceil(gridSpanY / gridStep)) + 1;

        scratchNormalGridHeights_.resize(static_cast<size_t>(gridCountX)
                                         * gridCountY);
        for (int gy = 0; gy < gridCountY; ++gy)
        {
            for (int gx = 0; gx < gridCountX; ++gx)
            {
                const glm::vec2 worldXY = gridOrigin
                    + glm::vec2(static_cast<float>(gx), static_cast<float>(gy))
                        * gridStep;
                scratchNormalGridHeights_[gy * gridCountX + gx] =
                    map.GetHeightAt(worldXY);
            }
        }

        scratchNormalGridNormals_.resize(scratchNormalGridHeights_.size());
        for (int gy = 0; gy < gridCountY; ++gy)
        {
            for (int gx = 0; gx < gridCountX; ++gx)
            {
                const int xL = std::max(gx - 1, 0);
                const int xR = std::min(gx + 1, gridCountX - 1);
                const int yD = std::max(gy - 1, 0);
                const int yU = std::min(gy + 1, gridCountY - 1);

                const float hL =
                    scratchNormalGridHeights_[gy * gridCountX + xL];
                const float hR =
                    scratchNormalGridHeights_[gy * gridCountX + xR];
                const float hD =
                    scratchNormalGridHeights_[yD * gridCountX + gx];
                const float hU =
                    scratchNormalGridHeights_[yU * gridCountX + gx];

                scratchNormalGridNormals_[gy * gridCountX + gx] =
                    glm::normalize(
                        glm::vec3(hL - hR, hD - hU, 2.0f * gridStep));
            }
        }

        auto sampleNormalGrid = [&](glm::vec2 worldXY) -> glm::vec3 {
            const glm::vec2 coordF = (worldXY - gridOrigin) / gridStep;
            const int gx = std::clamp(static_cast<int>(std::floor(coordF.x)), 0,
                                      gridCountX - 2);
            const int gy = std::clamp(static_cast<int>(std::floor(coordF.y)), 0,
                                      gridCountY - 2);
            const glm::vec2 frac = coordF
                - glm::vec2(static_cast<float>(gx), static_cast<float>(gy));

            const glm::vec3& n00 =
                scratchNormalGridNormals_[gy * gridCountX + gx];
            const glm::vec3& n10 =
                scratchNormalGridNormals_[gy * gridCountX + gx + 1];
            const glm::vec3& n01 =
                scratchNormalGridNormals_[(gy + 1) * gridCountX + gx];
            const glm::vec3& n11 =
                scratchNormalGridNormals_[(gy + 1) * gridCountX + gx + 1];

            return glm::normalize(glm::mix(glm::mix(n00, n10, frac.x),
                                           glm::mix(n01, n11, frac.x), frac.y));
        };

        scratchRingData_.resize(scratchHeights_.size());
        scratchNormalData_.resize(scratchHeights_.size());

        for (uint32_t y = 0; y < texSize; ++y)
        {
            for (uint32_t x = 0; x < texSize; ++x)
            {
                const float h = scratchHeights_[y * texSize + x];
                const glm::vec2 worldXY = snappedOrigin
                    + glm::vec2(static_cast<float>(x), static_cast<float>(y))
                        * texelSize;

                const glm::vec3 fixedNormal = sampleNormalGrid(worldXY);
                const float slope = 1.0f - fixedNormal.z;

                const float jitter =
                    MacroNoise(worldXY / 24.0f + glm::vec2(5.2f, 9.8f)) - 0.5f;

                const float rockByNoise =
                    glm::smoothstep(0.6f, 0.85f,
                                    MacroNoise(worldXY / kRockPatchScale
                                               + glm::vec2(37.1f, 58.9f)));
                const float rockBySlope =
                    glm::smoothstep(0.5f + jitter * 0.1f, 0.85f, slope);
                const float rockWeight = std::max(rockBySlope, rockByNoise);

                const float snowRetention =
                    1.0f - glm::smoothstep(0.5f, 0.9f, slope);
                const float snowByHeight = glm::smoothstep(
                    200.0f + jitter * 30.0f, 320.0f + jitter * 30.0f, h);
                const float snowWeight =
                    snowByHeight * snowRetention * (1.0f - rockWeight * 0.3f);

                const float dirtByNoise =
                    glm::smoothstep(0.55f + jitter * 0.1f, 0.8f,
                                    MacroNoise(worldXY / kDirtPatchScale
                                               + glm::vec2(91.7f, 12.3f)));
                const float dirtWeight =
                    dirtByNoise * (1.0f - rockWeight) * (1.0f - snowWeight);

                scratchRingData_[y * texSize + x] =
                    glm::vec4(h, rockWeight, snowWeight, dirtWeight);
                scratchNormalData_[y * texSize + x] = fixedNormal;
            }
        }

        glTextureSubImage3D(
            heightRingArray_, 0, 0, 0, static_cast<GLint>(ringIndex),
            static_cast<GLsizei>(texSize), static_cast<GLsizei>(texSize), 1,
            GL_RGBA, GL_FLOAT, glm::value_ptr(scratchRingData_[0]));

        glTextureSubImage3D(
            normalRingArray_, 0, 0, 0, static_cast<GLint>(ringIndex),
            static_cast<GLsizei>(texSize), static_cast<GLsizei>(texSize), 1,
            GL_RGB, GL_FLOAT, glm::value_ptr(scratchNormalData_[0]));
    }

    void GLTerrainRenderer::DrawRing(uint32_t ringIndex,
                                     const Shader* shaderBase) const
    {
        const GlShader* shader = dynamic_cast<const GlShader*>(shaderBase);

        shader->setValue("ringOrigin", ringStates_[ringIndex].snappedOrigin);
        shader->setValue("ringWorldSize", GetRingWorldSize(ringIndex));
        shader->setValue("ringLayer", static_cast<int>(ringIndex));

        const bool hasNextRing = ringIndex + 1 < kRingCount;
        shader->setValue("hasNextRing", hasNextRing);
        if (hasNextRing)
        {
            shader->setValue("nextRingOrigin",
                             ringStates_[ringIndex + 1].snappedOrigin);
            shader->setValue("nextRingWorldSize",
                             GetRingWorldSize(ringIndex + 1));
        }

        const bool hasInnerRing = ringIndex > 0;
        shader->setValue("hasInnerRing", hasInnerRing);
        if (hasInnerRing)
        {
            shader->setValue("innerRingOrigin",
                             ringStates_[ringIndex - 1].snappedOrigin);
            shader->setValue("innerRingWorldSize",
                             GetRingWorldSize(ringIndex - 1));
        }

        glBindVertexArray(solidVao_);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(solidIndexCount_),
                       GL_UNSIGNED_INT, 0);
    }

    void GLTerrainRenderer::render(const terrain::Map& map)
    {
        if (!Camera::get_active_camera())
        {
            Logger::LogError("Cant render terrain with no active camera!");
            return;
        }

        Camera* cam = Camera::get_active_camera();
        cam->calculate_matrices();

        const glm::vec3 cameraPos = cam->get_position();
        const glm::vec2 cameraXY(cameraPos.x, cameraPos.y);

        for (uint32_t ringIndex = 0; ringIndex < kRingCount; ++ringIndex)
        {
            const glm::vec2 snapped = ComputeSnappedOrigin(ringIndex, cameraXY);
            RingState& state = ringStates_[ringIndex];

            if (!state.initialized || snapped != state.snappedOrigin)
            {
                state.snappedOrigin = snapped;
                state.initialized = true;
                UpdateRingTexture(ringIndex, map, snapped);
            }
        }

        const GlShader* shader =
            dynamic_cast<const GlShader*>(terrainShader_.get());
        shader->use();

        shader->setValue("projection", cam->get_projection_matrix_());
        shader->setValue("view", cam->get_view_matrix_());
        shader->setValue("viewPos", cameraPos);
        shader->setValue("ringTexelCount", static_cast<float>(kRingResolution));
        shader->setValue("fogColor", kFogColor);
        shader->setValue("fogDensity", kFogDensity);

        for (size_t i = 0; i < kMaterials.size(); ++i)
        {
            shader->setValue(
                ("materialTileSize[" + std::to_string(i) + "]").c_str(),
                kMaterials[i].tileSizeMeters);
        }

        const auto* dirLight = ShadowRenderer::GetDirectionalLight();
        if (dirLight)
        {
            shader->setValue("lightDir",
                             Utils::GetForwardFromModelMatrix(
                                 dirLight->parent_object->get_transform()
                                     .get_model_matrix()));
            shader->setValue("lightColor", dirLight->get_color());
            shader->setValue("lightIntensity",
                             std::numbers::pi_v<float> * dirLight->intensity_);
        }
        else
        {
            shader->setValue("lightDir", glm::vec3(0.0f, 0.0f, -1.0f));
            shader->setValue("lightColor", glm::vec3(1.0f));
            shader->setValue("lightIntensity", 1.0f);
        }

        shader->setValue(
            "cascadeCount",
            static_cast<int>(ShadowRenderer::shadowCascadeLevels.size()));
        for (size_t i = 0; i < ShadowRenderer::shadowCascadeLevels.size(); ++i)
        {
            shader->setValue(
                ("cascadePlaneDistances[" + std::to_string(i) + "]").c_str(),
                cam->get_farPlane() / ShadowRenderer::shadowCascadeLevels[i]);
        }

        glBindTextureUnit(
            6,
            dynamic_cast<GLShadowRenderer*>(ShadowRenderer::getInstance())
                ->depthMap);
        shader->setValue("shadowMap", 6);

        GL_Skybox* skybox = dynamic_cast<GL_Skybox*>(Skybox::getInstance());
        bool haveSkybox = skybox && skybox->is_loaded();
        shader->setValue("haveSkybox", haveSkybox);

        if (haveSkybox)
        {
            glBindTextureUnit(7, skybox->get_irradiance_map());
            shader->setValue("irradianceMap", 7);
            shader->setValue("ambientIntensity",
                             skybox->get_ambient_intensity());
        }

        glBindTextureUnit(0, heightRingArray_);
        shader->setValue("heightRingArray", 0);

        glBindTextureUnit(1, normalRingArray_);
        shader->setValue("normalRingArray", 1);

        glBindTextureUnit(2, materialGaussianAlbedoArray_);
        shader->setValue("materialGaussianAlbedoArray", 2);

        glBindTextureUnit(3, materialAlbedoLutArray_);
        shader->setValue("materialAlbedoLutArray", 3);

        glBindTextureUnit(4, materialNormalArray_);
        shader->setValue("materialNormalArray", 4);

        glBindTextureUnit(5, materialOrmArray_);
        shader->setValue("materialOrmArray", 5);

        glEnable(GL_POLYGON_OFFSET_FILL);

        for (uint32_t i = 0; i < kRingCount; ++i)
        {
            const uint32_t ringIndex = kRingCount - 1 - i;

            const float biasUnits =
                -static_cast<float>((kRingCount - 1) - ringIndex);
            glPolygonOffset(0.0f, biasUnits);

            DrawRing(ringIndex, terrainShader_.get());
        }

        glDisable(GL_POLYGON_OFFSET_FILL);
    }
} // namespace raphEngine::graphics::ogl
