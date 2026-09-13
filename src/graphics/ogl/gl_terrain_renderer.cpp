#include "graphics/ogl/gl_terrain_renderer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <RaphEngine2/component/camera_component.hpp>
#include <RaphEngine2/default_shaders.hpp>
#include <RaphEngine2/graphics/ogl/gl_shader.hpp>
#include <RaphEngine2/graphics/shader.hpp>
#include <RaphEngine2/graphics/shadow_renderer.hpp>
#include <RaphEngine2/graphics/skybox.hpp>
#include <RaphEngine2/graphics/stochastic_texture_baker.hpp>
#include <RaphEngine2/graphics/texture_loader.hpp>
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/terrain/chunk.hpp>
#include <RaphEngine2/terrain/map.hpp>
#include <RaphEngine2/utils.hpp>

#include "graphics/ogl/gl_shadow_renderer.hpp"
#include "graphics/ogl/gl_skybox.hpp"

namespace raphEngine::graphics::ogl
{
    namespace
    {
        uint64_t PackCoord(glm::ivec2 coord)
        {
            return (static_cast<uint64_t>(static_cast<uint32_t>(coord.x)) << 32)
                | static_cast<uint32_t>(coord.y);
        }

        struct ChunkInstanceGpuData
        {
            glm::vec2 worldOrigin;
            glm::vec2 heightRange;
            uint32_t textureLayer;
            float pad0;
        };

        static_assert(sizeof(ChunkInstanceGpuData) == 24,
                      "ChunkInstanceGpuData must match the std430 layout the "
                      "terrain shader expects");

        struct TerrainMaterialDefinition
        {
            const char* albedoPath;
            const char* normalPath;
            const char* ormPath; // R = AO, G = roughness, B = metallic
            float tileSizeMeters;
        };

        constexpr int kAlbedoLutResolution = 256;

        constexpr std::array<TerrainMaterialDefinition, 4> kMaterials = { {
            { "assets/textures/terrain/forrest_ground_01_diff_4k.jpg",
              "assets/textures/terrain/forrest_ground_01_nor_gl_4k.jpg",
              "assets/textures/terrain/forrest_ground_01_arm_4k.jpg", 2.0f },

            { "assets/textures/terrain/rocks_ground_04_diff_4k.jpg",
              "assets/textures/terrain/rocks_ground_04_nor_gl_4k.jpg",
              "assets/textures/terrain/rocks_ground_04_arm_4k.jpg", 2.0f },

            { "assets/textures/terrain/snow_02_diff_4k.jpg",
              "assets/textures/terrain/snow_02_nor_gl_4k.jpg",
              "assets/textures/terrain/snow_02_arm_4k.jpg", 2.0f },

            { "assets/textures/terrain/forest_ground_05_diff_4k.jpg",
              "assets/textures/terrain/forest_ground_05_nor_gl_4k.jpg",
              "assets/textures/terrain/forest_ground_05_arm_4k.jpg", 2.0f },
        } };
    } // namespace

    GLTerrainRenderer::GLTerrainRenderer()
    {
        CreatePatchMesh();
        CreateHeightTextureArray();
        CreatePaintMaskTextureArray();
        CreateMaterialTextureArrays();
        CreateInstanceBuffer();

        terrainShader_ =
            Shader::loadShader(ShaderStages{ .vertex = terrain_vs_shader,
                                             .tessControl = terrain_tcs_shader,
                                             .tessEval = terrain_tes_shader,
                                             .fragment = terrain_fs_shader });
    }

    GLTerrainRenderer::~GLTerrainRenderer()
    {
        if (chunkInstanceSsboPtr_ != nullptr)
        {
            glUnmapNamedBuffer(chunkInstanceSsbo_);
        }

        glDeleteBuffers(1, &chunkInstanceSsbo_);
        glDeleteTextures(1, &materialOrmArray_);
        glDeleteTextures(1, &materialNormalArray_);
        glDeleteTextures(1, &materialAlbedoLutArray_);
        glDeleteTextures(1, &materialGaussianAlbedoArray_);
        glDeleteTextures(1, &paintMaskTextureArray_);
        glDeleteTextures(1, &heightTextureArray_);
        glDeleteBuffers(1, &patchEbo_);
        glDeleteBuffers(1, &patchVbo_);
        glDeleteVertexArrays(1, &patchVao_);
    }

    void GLTerrainRenderer::CreatePatchMesh()
    {
        std::vector<glm::vec2> vertices;
        vertices.reserve((kPatchGridSize + 1) * (kPatchGridSize + 1));

        for (uint32_t y = 0; y <= kPatchGridSize; ++y)
        {
            for (uint32_t x = 0; x <= kPatchGridSize; ++x)
            {
                vertices.emplace_back(static_cast<float>(x) / kPatchGridSize,
                                      static_cast<float>(y) / kPatchGridSize);
            }
        }

        std::vector<uint32_t> indices;
        indices.reserve(kPatchGridSize * kPatchGridSize * 4);

        const uint32_t rowStride = kPatchGridSize + 1;
        for (uint32_t y = 0; y < kPatchGridSize; ++y)
        {
            for (uint32_t x = 0; x < kPatchGridSize; ++x)
            {
                const uint32_t i0 = y * rowStride + x;
                const uint32_t i1 = i0 + 1;
                const uint32_t i2 = i0 + rowStride;
                const uint32_t i3 = i2 + 1;

                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i3);
                indices.push_back(i2);
            }
        }

        patchCount_ = static_cast<uint32_t>(indices.size() / 4);

        glCreateVertexArrays(1, &patchVao_);
        glCreateBuffers(1, &patchVbo_);
        glCreateBuffers(1, &patchEbo_);

        glNamedBufferStorage(
            patchVbo_,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec2)),
            vertices.data(), 0);
        glNamedBufferStorage(
            patchEbo_,
            static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
            indices.data(), 0);

        glVertexArrayVertexBuffer(patchVao_, 0, patchVbo_, 0,
                                  sizeof(glm::vec2));
        glVertexArrayElementBuffer(patchVao_, patchEbo_);

        glEnableVertexArrayAttrib(patchVao_, 0);
        glVertexArrayAttribFormat(patchVao_, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(patchVao_, 0, 0);
    }

    void GLTerrainRenderer::CreateHeightTextureArray()
    {
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &heightTextureArray_);
        glTextureStorage3D(heightTextureArray_, 1, GL_R16,
                           static_cast<GLsizei>(terrain::kChunkResolution),
                           static_cast<GLsizei>(terrain::kChunkResolution),
                           static_cast<GLsizei>(kMaxResidentChunks));

        glTextureParameteri(heightTextureArray_, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR);
        glTextureParameteri(heightTextureArray_, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);
        glTextureParameteri(heightTextureArray_, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
        glTextureParameteri(heightTextureArray_, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);

        layerInUse_.assign(kMaxResidentChunks, false);
    }

    void GLTerrainRenderer::CreatePaintMaskTextureArray()
    {
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &paintMaskTextureArray_);
        glTextureStorage3D(paintMaskTextureArray_, 1, GL_R8UI,
                           static_cast<GLsizei>(terrain::kChunkResolution),
                           static_cast<GLsizei>(terrain::kChunkResolution),
                           static_cast<GLsizei>(kMaxResidentChunks));

        glTextureParameteri(paintMaskTextureArray_, GL_TEXTURE_MIN_FILTER,
                            GL_NEAREST);
        glTextureParameteri(paintMaskTextureArray_, GL_TEXTURE_MAG_FILTER,
                            GL_NEAREST);
        glTextureParameteri(paintMaskTextureArray_, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
        glTextureParameteri(paintMaskTextureArray_, GL_TEXTURE_WRAP_T,
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

    void GLTerrainRenderer::CreateInstanceBuffer()
    {
        glCreateBuffers(1, &chunkInstanceSsbo_);

        const GLbitfield flags =
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        const GLsizeiptr bufferSize = static_cast<GLsizeiptr>(
            kMaxResidentChunks * sizeof(ChunkInstanceGpuData));

        glNamedBufferStorage(chunkInstanceSsbo_, bufferSize, nullptr, flags);
        chunkInstanceSsboPtr_ =
            glMapNamedBufferRange(chunkInstanceSsbo_, 0, bufferSize, flags);

        if (chunkInstanceSsboPtr_ == nullptr)
        {
            Logger::LogError("GLTerrainRenderer: failed to persistently map "
                             "the chunk instance buffer");
        }
    }

    GLTerrainRenderer::LayerAcquireResult
    GLTerrainRenderer::AcquireTextureLayer(glm::ivec2 gridCoord,
                                           uint64_t currentVersion)
    {
        const uint64_t key = PackCoord(gridCoord);
        auto it = residentChunks_.find(key);
        if (it != residentChunks_.end())
        {
            it->second.inUseThisFrame = true;
            const bool needsUpload =
                it->second.uploadedVersion != currentVersion;
            if (needsUpload)
            {
                it->second.uploadedVersion = currentVersion;
            }
            return { it->second.textureLayer, needsUpload };
        }

        for (uint32_t layer = 0; layer < layerInUse_.size(); ++layer)
        {
            if (!layerInUse_[layer])
            {
                layerInUse_[layer] = true;
                residentChunks_[key] =
                    ChunkGpuSlot{ layer, currentVersion, true };
                return { layer, true };
            }
        }

        Logger::LogError("GLTerrainRenderer: exceeded kMaxResidentChunks, "
                         "terrain will show gaps");
        return { 0, false };
    }

    void GLTerrainRenderer::ReleaseUnusedLayers()
    {
        for (auto it = residentChunks_.begin(); it != residentChunks_.end();)
        {
            if (!it->second.inUseThisFrame)
            {
                layerInUse_[it->second.textureLayer] = false;
                it = residentChunks_.erase(it);
            }
            else
            {
                it->second.inUseThisFrame = false;
                ++it;
            }
        }
    }

    void GLTerrainRenderer::UploadChunkHeights(const terrain::Chunk& chunk,
                                               uint32_t layer)
    {
        const std::span<const uint16_t> heights =
            chunk.GetHeightDataForUpload();

        glTextureSubImage3D(heightTextureArray_, 0, 0, 0,
                            static_cast<GLint>(layer),
                            static_cast<GLsizei>(terrain::kChunkResolution),
                            static_cast<GLsizei>(terrain::kChunkResolution), 1,
                            GL_RED, GL_UNSIGNED_SHORT, heights.data());
    }

    void GLTerrainRenderer::UploadChunkPaintMask(const terrain::Chunk& chunk,
                                                 uint32_t layer)
    {
        const std::span<const uint8_t> paintMask =
            chunk.GetPaintDataForUpload();

        glTextureSubImage3D(paintMaskTextureArray_, 0, 0, 0,
                            static_cast<GLint>(layer),
                            static_cast<GLsizei>(terrain::kChunkResolution),
                            static_cast<GLsizei>(terrain::kChunkResolution), 1,
                            GL_RED_INTEGER, GL_UNSIGNED_BYTE, paintMask.data());
    }

    void GLTerrainRenderer::render(const terrain::Map& map)
    {
        if (!Camera::get_active_camera())
        {
            Logger::LogError("Cant render terrain with no active camera!");
            return;
        }

        if (chunkInstanceSsboPtr_ == nullptr)
        {
            return;
        }

        Camera* cam = Camera::get_active_camera();
        cam->calculate_matrices();

        const glm::vec3 cameraPos = cam->get_position();

        struct VisibleChunk
        {
            glm::ivec2 coord;
            const terrain::Chunk* chunk;
            float distanceSq;
        };

        std::vector<VisibleChunk> visible;
        const glm::ivec2 gridSize = map.GetGridSize();

        for (int y = 0; y < gridSize.y; ++y)
        {
            for (int x = 0; x < gridSize.x; ++x)
            {
                const glm::ivec2 coord(x, y);
                if (!map.IsChunkResident(coord))
                {
                    continue;
                }

                const terrain::Chunk* chunk = map.GetChunkAt(coord);
                const glm::vec2 chunkCenter = map.GridCoordToWorldOrigin(coord)
                    + glm::vec2(static_cast<float>(terrain::kChunkResolution)
                                * 0.5f);

                const glm::vec2 diff =
                    chunkCenter - glm::vec2(cameraPos.x, cameraPos.y);
                visible.push_back({ coord, chunk, glm::dot(diff, diff) });
            }
        }

        std::sort(visible.begin(), visible.end(),
                  [](const VisibleChunk& a, const VisibleChunk& b) {
                      return a.distanceSq < b.distanceSq;
                  });

        if (visible.size() > kMaxResidentChunks)
        {
            visible.resize(kMaxResidentChunks);
        }

        auto* instanceData =
            static_cast<ChunkInstanceGpuData*>(chunkInstanceSsboPtr_);
        uint32_t instanceCount = 0;

        for (const VisibleChunk& v : visible)
        {
            const LayerAcquireResult acquired =
                AcquireTextureLayer(v.coord, v.chunk->GetGpuDataVersion());
            if (acquired.needsUpload)
            {
                UploadChunkHeights(*v.chunk, acquired.layer);
                UploadChunkPaintMask(*v.chunk, acquired.layer);
            }

            instanceData[instanceCount].worldOrigin =
                map.GridCoordToWorldOrigin(v.coord);
            instanceData[instanceCount].heightRange =
                v.chunk->GetWorldHeightRange();
            instanceData[instanceCount].textureLayer = acquired.layer;

            ++instanceCount;
        }

        ReleaseUnusedLayers();

        if (instanceCount == 0)
        {
            return;
        }

        const GlShader* shader =
            dynamic_cast<const GlShader*>(terrainShader_.get());
        shader->use();

        shader->setValue("projection", cam->get_projection_matrix_());
        shader->setValue("view", cam->get_view_matrix_());
        shader->setValue("viewPos", cameraPos);
        shader->setValue("chunkResolution",
                         static_cast<float>(terrain::kChunkResolution));

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
            shader->setValue("lightIntensity", 0.0f);
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

            glBindTextureUnit(8, skybox->get_prefilter_map());
            shader->setValue("prefilterMap", 8);
            shader->setValue("maxPrefilterLod", GL_Skybox::kMaxPrefilterLod);

            glBindTextureUnit(9, skybox->get_brdf_lut());
            shader->setValue("brdfLUT", 9);

            shader->setValue("ambientIntensity",
                             skybox->get_ambient_intensity());
            shader->setValue("reflectionExposure",
                             skybox->get_reflection_exposure());
        }
        else
        {
            shader->setValue("reflectionExposure", 1.0f);
        }

        glBindTextureUnit(0, heightTextureArray_);
        shader->setValue("heightMapArray", 0);

        glBindTextureUnit(1, paintMaskTextureArray_);
        shader->setValue("paintMaskArray", 1);

        glBindTextureUnit(2, materialGaussianAlbedoArray_);
        shader->setValue("materialGaussianAlbedoArray", 2);

        glBindTextureUnit(3, materialAlbedoLutArray_);
        shader->setValue("materialAlbedoLutArray", 3);

        glBindTextureUnit(4, materialNormalArray_);
        shader->setValue("materialNormalArray", 4);

        glBindTextureUnit(5, materialOrmArray_);
        shader->setValue("materialOrmArray", 5);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, chunkInstanceSsbo_);

        glPatchParameteri(GL_PATCH_VERTICES, 4);

        glBindVertexArray(patchVao_);
        glDrawElementsInstanced(
            GL_PATCHES, static_cast<GLsizei>(patchCount_ * 4), GL_UNSIGNED_INT,
            0, static_cast<GLsizei>(instanceCount));
    }
} // namespace raphEngine::graphics::ogl
