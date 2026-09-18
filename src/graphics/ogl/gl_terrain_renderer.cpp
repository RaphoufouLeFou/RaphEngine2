#include "graphics/ogl/gl_terrain_renderer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tbb/parallel_for.h>

#include <RaphEngine2/component/camera_component.hpp>

#include <RaphEngine2/default_shaders.hpp>
#include <RaphEngine2/graphics/ogl/gl_shader.hpp>
#include <RaphEngine2/graphics/shader.hpp>
#include <RaphEngine2/graphics/fog_settings.hpp>
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
    } // namespace

    GLTerrainRenderer::GLTerrainRenderer()
    {
        CreateNodeMesh();
        CreateInstanceBuffer();
        CreateHeightNodeArray();
        CreateNormalNodeArray();
        CreatePaintNodeArray();
        CreateMaterialTextureArrays();

        terrainShader_ = Shader::loadShader(
            ShaderStages{ .vertex = terrain_quad_vs_shader,
                          .fragment = terrain_quad_fs_shader });
        terrainShader_->bindUniformBlock("LightSpaceMatrices", 0);

        terrainShadowShader_ = Shader::loadShader(
            ShaderStages{ .vertex = terrain_shadow_vs_shader,
                          .fragment = terrain_shadow_fs_shader });
    }

    GLTerrainRenderer::~GLTerrainRenderer()
    {
        glDeleteTextures(1, &materialOrmArray_);
        glDeleteTextures(1, &materialNormalArray_);
        glDeleteTextures(1, &materialAlbedoLutArray_);
        glDeleteTextures(1, &materialGaussianAlbedoArray_);
        glDeleteTextures(1, &paintNodeArray_);
        glDeleteTextures(1, &normalNodeArray_);
        glDeleteTextures(1, &heightNodeArray_);
        glDeleteBuffers(1, &nodeInstanceBuffer_);
        glDeleteBuffers(1, &nodeEbo_);
        glDeleteVertexArrays(1, &nodeVao_);
        glDeleteBuffers(1, &nodeVertexBuffer_);
    }

    void GLTerrainRenderer::CreateNodeMesh()
    {
        const uint32_t N = kNodeResolution;
        const uint32_t rowStride = N + 1;
        const uint32_t topVertexCount = rowStride * rowStride;

        std::vector<glm::vec4> vertices;
        vertices.reserve(topVertexCount + 4 * rowStride);

        for (uint32_t y = 0; y <= N; ++y)
            for (uint32_t x = 0; x <= N; ++x)
                vertices.emplace_back(static_cast<float>(x) / N,
                                      static_cast<float>(y) / N, 0.0f, 0.0f);

        auto topIndex = [&](uint32_t x, uint32_t y) {
            return y * rowStride + x;
        };

        const uint32_t leftSkirtStart = topVertexCount;
        for (uint32_t y = 0; y <= N; ++y)
            vertices.emplace_back(0.0f, static_cast<float>(y) / N, -1.0f, 0.0f);

        const uint32_t rightSkirtStart = leftSkirtStart + rowStride;
        for (uint32_t y = 0; y <= N; ++y)
            vertices.emplace_back(1.0f, static_cast<float>(y) / N, 1.0f, 0.0f);

        const uint32_t bottomSkirtStart = rightSkirtStart + rowStride;
        for (uint32_t x = 0; x <= N; ++x)
            vertices.emplace_back(static_cast<float>(x) / N, 0.0f, 0.0f, -1.0f);

        const uint32_t topSkirtStart = bottomSkirtStart + rowStride;
        for (uint32_t x = 0; x <= N; ++x)
            vertices.emplace_back(static_cast<float>(x) / N, 1.0f, 0.0f, 1.0f);

        std::vector<uint32_t> indices;
        indices.reserve(static_cast<size_t>(N) * N * 6 + 4 * N * 6);

        for (uint32_t y = 0; y < N; ++y)
        {
            for (uint32_t x = 0; x < N; ++x)
            {
                uint32_t i0 = topIndex(x, y);
                uint32_t i1 = topIndex(x + 1, y);
                uint32_t i2 = topIndex(x, y + 1);
                uint32_t i3 = topIndex(x + 1, y + 1);
                indices.insert(indices.end(), { i0, i1, i2, i1, i3, i2 });
            }
        }

        auto addSkirtStrip = [&](auto topIndexFn, uint32_t skirtStart) {
            for (uint32_t j = 0; j < N; ++j)
            {
                uint32_t topA = topIndexFn(j);
                uint32_t topB = topIndexFn(j + 1);
                uint32_t skirtA = skirtStart + j;
                uint32_t skirtB = skirtStart + j + 1;
                indices.insert(indices.end(),
                               { topA, topB, skirtA, topB, skirtB, skirtA });
            }
        };

        addSkirtStrip([&](uint32_t j) { return topIndex(0, j); },
                      leftSkirtStart);
        addSkirtStrip([&](uint32_t j) { return topIndex(N, j); },
                      rightSkirtStart);
        addSkirtStrip([&](uint32_t j) { return topIndex(j, 0); },
                      bottomSkirtStart);
        addSkirtStrip([&](uint32_t j) { return topIndex(j, N); },
                      topSkirtStart);

        nodeIndexCount_ = static_cast<uint32_t>(indices.size());

        glCreateBuffers(1, &nodeVertexBuffer_);
        glNamedBufferStorage(
            nodeVertexBuffer_,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec4)),
            vertices.data(), 0);

        glCreateVertexArrays(1, &nodeVao_);
        glCreateBuffers(1, &nodeEbo_);
        glNamedBufferStorage(
            nodeEbo_,
            static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
            indices.data(), 0);
        glVertexArrayVertexBuffer(nodeVao_, 0, nodeVertexBuffer_, 0,
                                  sizeof(glm::vec4));
        glVertexArrayElementBuffer(nodeVao_, nodeEbo_);
        glEnableVertexArrayAttrib(nodeVao_, 0);
        glVertexArrayAttribFormat(nodeVao_, 0, 4, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(nodeVao_, 0, 0);
    }

    void GLTerrainRenderer::CreateInstanceBuffer()
    {
        glCreateBuffers(1, &nodeInstanceBuffer_);
        glNamedBufferStorage(
            nodeInstanceBuffer_,
            static_cast<GLsizeiptr>(kMaxActiveNodes * sizeof(glm::vec4)),
            nullptr, GL_DYNAMIC_STORAGE_BIT);

        glVertexArrayVertexBuffer(nodeVao_, 1, nodeInstanceBuffer_, 0,
                                  sizeof(glm::vec4));
        glVertexArrayBindingDivisor(nodeVao_, 1, 1);
        glEnableVertexArrayAttrib(nodeVao_, 1);
        glVertexArrayAttribFormat(nodeVao_, 1, 4, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(nodeVao_, 1, 1);
    }

    void GLTerrainRenderer::CreateHeightNodeArray()
    {
        const GLsizei texSize = static_cast<GLsizei>(kNodeResolution + 1);

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &heightNodeArray_);
        glTextureStorage3D(heightNodeArray_, 1, GL_RGBA32F, texSize, texSize,
                           static_cast<GLsizei>(kMaxActiveNodes));

        glTextureParameteri(heightNodeArray_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(heightNodeArray_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(heightNodeArray_, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
        glTextureParameteri(heightNodeArray_, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);

        layerInUse_.assign(kMaxActiveNodes, false);
    }

    void GLTerrainRenderer::CreateNormalNodeArray()
    {
        const GLsizei texSize = static_cast<GLsizei>(kNodeResolution + 1);

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &normalNodeArray_);
        glTextureStorage3D(normalNodeArray_, 1, GL_RGB16F, texSize, texSize,
                           static_cast<GLsizei>(kMaxActiveNodes));

        glTextureParameteri(normalNodeArray_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(normalNodeArray_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(normalNodeArray_, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
        glTextureParameteri(normalNodeArray_, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);
    }

    void GLTerrainRenderer::CreatePaintNodeArray()
    {
        const GLsizei texSize = static_cast<GLsizei>(kNodeResolution + 1);

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &paintNodeArray_);
        glTextureStorage3D(paintNodeArray_, 1, GL_R8UI, texSize, texSize,
                           static_cast<GLsizei>(kMaxActiveNodes));

        glTextureParameteri(paintNodeArray_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(paintNodeArray_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(paintNodeArray_, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
        glTextureParameteri(paintNodeArray_, GL_TEXTURE_WRAP_T,
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

        std::vector<graphics::TextureLoader::RawTexture> rawTextures;
        rawTextures.reserve(kMaterials.size());

        for (const TerrainMaterialDefinition& material : kMaterials)
        {
            graphics::TextureLoader::RawTexture raw =
                loader->load_texture_raw(material.albedoPath);
            if (raw.data == nullptr)
            {
                for (auto& r : rawTextures)
                    loader->free_raw(r);
                throw std::runtime_error(
                    std::string("GLTerrainRenderer: failed to load terrain "
                                "albedo texture ")
                    + material.albedoPath);
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
                    "GLTerrainRenderer: all terrain albedo textures must share "
                    "resolution and channel count");
            }

            rawTextures.push_back(raw);
        }

        std::vector<graphics::StochasticTextureData> stochasticData(
            rawTextures.size());
        tbb::parallel_for(size_t(0), rawTextures.size(), [&](size_t i) {
            stochasticData[i] = graphics::ComputeStochasticTextureData(
                rawTextures[i].data, rawTextures[i].width,
                rawTextures[i].height, rawTextures[i].nrChannels,
                kAlbedoLutResolution);
        });

        for (auto& raw : rawTextures)
        {
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

    GLTerrainRenderer::QuadCandidate
    GLTerrainRenderer::MakeCandidate(int level, int x, int y,
                                     glm::vec2 cameraXY) const
    {
        const float size =
            kLeafWorldSize * static_cast<float>(1 << (kMaxLevel - level));
        const glm::vec2 origin(static_cast<float>(x) * size,
                               static_cast<float>(y) * size);
        const glm::vec2 closest =
            glm::clamp(cameraXY, origin, origin + glm::vec2(size));
        const float dist = glm::length(cameraXY - closest);

        return QuadCandidate{ QuadNode{ level, x, y, origin, size }, dist };
    }

    void GLTerrainRenderer::RasterizeLevelGrid(
        const std::vector<QuadNode>& leaves, int64_t& outMinX, int64_t& outMinY,
        int64_t& outWidth, int64_t& outHeight) const
    {
        int64_t minX = std::numeric_limits<int64_t>::max();
        int64_t minY = std::numeric_limits<int64_t>::max();
        int64_t maxX = std::numeric_limits<int64_t>::min();
        int64_t maxY = std::numeric_limits<int64_t>::min();

        for (const QuadNode& n : leaves)
        {
            const int64_t units = 1ll << (kMaxLevel - n.level);
            const int64_t x0 = static_cast<int64_t>(n.x) * units;
            const int64_t y0 = static_cast<int64_t>(n.y) * units;
            minX = std::min(minX, x0);
            minY = std::min(minY, y0);
            maxX = std::max(maxX, x0 + units);
            maxY = std::max(maxY, y0 + units);
        }

        outMinX = minX;
        outMinY = minY;
        outWidth = maxX - minX;
        outHeight = maxY - minY;

        scratchLevelGrid_.assign(static_cast<size_t>(outWidth * outHeight),
                                 static_cast<int16_t>(-1));

        for (const QuadNode& n : leaves)
        {
            const int64_t units = 1ll << (kMaxLevel - n.level);
            const int64_t x0 = static_cast<int64_t>(n.x) * units - minX;
            const int64_t y0 = static_cast<int64_t>(n.y) * units - minY;

            for (int64_t dy = 0; dy < units; ++dy)
            {
                int16_t* row = &scratchLevelGrid_[static_cast<size_t>(
                    (y0 + dy) * outWidth + x0)];
                std::fill(row, row + units, static_cast<int16_t>(n.level));
            }
        }
    }

    void GLTerrainRenderer::BalanceLeafSet(std::vector<QuadNode>& leaves) const
    {
        if (leaves.empty())
            return;

        size_t currentTotal = leaves.size();
        bool budgetExhausted = false;
        bool changed = true;

        while (changed)
        {
            changed = false;

            int64_t minX, minY, width, height;
            RasterizeLevelGrid(leaves, minX, minY, width, height);

            auto maxInStrip = [&](int64_t gx, int64_t gy0, int64_t gy1) -> int {
                if (gx < 0 || gx >= width)
                    return -1;
                int m = -1;
                for (int64_t y = std::max<int64_t>(gy0, 0);
                     y < std::min<int64_t>(gy1, height); ++y)
                    m = std::max<int>(
                        m,
                        scratchLevelGrid_[static_cast<size_t>(y * width + gx)]);
                return m;
            };
            auto maxInRow = [&](int64_t gy, int64_t gx0, int64_t gx1) -> int {
                if (gy < 0 || gy >= height)
                    return -1;
                int m = -1;
                for (int64_t x = std::max<int64_t>(gx0, 0);
                     x < std::min<int64_t>(gx1, width); ++x)
                    m = std::max<int>(
                        m,
                        scratchLevelGrid_[static_cast<size_t>(gy * width + x)]);
                return m;
            };

            std::vector<QuadNode> nextLeaves;
            nextLeaves.reserve(leaves.size());

            for (const QuadNode& node : leaves)
            {
                const int64_t units = 1ll << (kMaxLevel - node.level);
                const int64_t x0 = static_cast<int64_t>(node.x) * units - minX;
                const int64_t y0 = static_cast<int64_t>(node.y) * units - minY;

                const int worst =
                    std::max({ maxInStrip(x0 + units, y0, y0 + units),
                               maxInStrip(x0 - 1, y0, y0 + units),
                               maxInRow(y0 + units, x0, x0 + units),
                               maxInRow(y0 - 1, x0, x0 + units) });

                const bool needsSplit =
                    node.level < kMaxLevel && worst > node.level + 1;

                if (needsSplit && currentTotal + 3 > kMaxActiveNodes)
                {
                    budgetExhausted = true;
                    nextLeaves.push_back(node);
                    continue;
                }

                if (needsSplit)
                {
                    for (int cy = 0; cy < 2; ++cy)
                    {
                        for (int cx = 0; cx < 2; ++cx)
                        {
                            const int childLevel = node.level + 1;
                            const int childX = node.x * 2 + cx;
                            const int childY = node.y * 2 + cy;
                            const float childSize = node.size * 0.5f;
                            const glm::vec2 childOrigin = node.origin
                                + glm::vec2(static_cast<float>(cx),
                                            static_cast<float>(cy))
                                    * childSize;

                            nextLeaves.push_back({ childLevel, childX, childY,
                                                   childOrigin, childSize });
                        }
                    }
                    currentTotal += 3;
                    changed = true;
                }
                else
                {
                    nextLeaves.push_back(node);
                }
            }

            leaves = std::move(nextLeaves);
        }

        if (budgetExhausted)
        {
            Logger::LogWarning("GLTerrainRenderer: node budget too tight to "
                               "fully 2:1-balance the quadtree this frame; "
                               "some LOD boundaries may show a wider step than "
                               "usual. Consider raising kMaxActiveNodes.");
        }
    }

    void GLTerrainRenderer::BuildLeafSet(glm::vec2 cameraXY, glm::vec2 worldMin,
                                         glm::vec2 worldMax,
                                         std::vector<QuadNode>& outLeaves) const
    {
        outLeaves.clear();

        const float rootSize =
            kLeafWorldSize * static_cast<float>(1 << kMaxLevel);
        const int cameraRootX =
            static_cast<int>(std::floor(cameraXY.x / rootSize));
        const int cameraRootY =
            static_cast<int>(std::floor(cameraXY.y / rootSize));

        std::priority_queue<QuadCandidate, std::vector<QuadCandidate>,
                            QuadCandidateCompare>
            frontier;

        uint32_t activeLeafCount = 0;
        for (int ry = -1; ry <= 1; ++ry)
        {
            for (int rx = -1; rx <= 1; ++rx)
            {
                frontier.push(MakeCandidate(0, cameraRootX + rx,
                                            cameraRootY + ry, cameraXY));
                ++activeLeafCount;
            }
        }

        while (!frontier.empty())
        {
            QuadCandidate candidate = frontier.top();
            frontier.pop();

            const glm::vec2 nodeMin = candidate.node.origin;
            const glm::vec2 nodeMax =
                candidate.node.origin + glm::vec2(candidate.node.size);
            const bool fullyOutside = nodeMax.x <= worldMin.x
                || nodeMin.x >= worldMax.x || nodeMax.y <= worldMin.y
                || nodeMin.y >= worldMax.y;

            if (fullyOutside)
            {
                continue;
            }

            const bool tooDeep = candidate.node.level >= kMaxLevel;
            const bool farEnough = candidate.distance
                >= candidate.node.size * kSplitDistanceFactor;
            const bool wouldExceedBudget =
                activeLeafCount + 3 > kMaxActiveNodes;

            if (tooDeep || farEnough || wouldExceedBudget)
            {
                outLeaves.push_back(candidate.node);
                continue;
            }

            activeLeafCount += 3;
            for (int cy = 0; cy < 2; ++cy)
            {
                for (int cx = 0; cx < 2; ++cx)
                {
                    frontier.push(MakeCandidate(
                        candidate.node.level + 1, candidate.node.x * 2 + cx,
                        candidate.node.y * 2 + cy, cameraXY));
                }
            }
        }

        BalanceLeafSet(outLeaves);
    }

    void GLTerrainRenderer::InvalidateAllNodes()
    {
        for (const auto& [key, resident] : residentNodes_)
        {
            layerInUse_[resident.layer] = false;
        }
        residentNodes_.clear();
    }

    void GLTerrainRenderer::BuildNodeData(const QuadNode& node,
                                          const terrain::Map& map,
                                          uint32_t layer)
    {
        const uint32_t texSize = kNodeResolution + 1;
        const float texelSize = node.size / static_cast<float>(kNodeResolution);

        const float gridStep = std::max(kFixedNormalSampleDistance, texelSize);
        constexpr int kGridMargin = 1;
        const int gridCount = static_cast<int>(std::ceil(node.size / gridStep))
            + 1 + kGridMargin * 2;
        const glm::vec2 gridOrigin =
            node.origin - glm::vec2(gridStep * static_cast<float>(kGridMargin));

        std::vector<float> gridHeights(static_cast<size_t>(gridCount)
                                       * gridCount);
        for (int gy = 0; gy < gridCount; ++gy)
        {
            for (int gx = 0; gx < gridCount; ++gx)
            {
                const glm::vec2 worldXY = gridOrigin
                    + glm::vec2(static_cast<float>(gx), static_cast<float>(gy))
                        * gridStep;
                gridHeights[gy * gridCount + gx] = map.GetHeightAt(worldXY);
            }
        }

        std::vector<glm::vec3> gridNormals(gridHeights.size());
        for (int gy = 0; gy < gridCount; ++gy)
        {
            for (int gx = 0; gx < gridCount; ++gx)
            {
                const int xL = std::max(gx - 1, 0);
                const int xR = std::min(gx + 1, gridCount - 1);
                const int yD = std::max(gy - 1, 0);
                const int yU = std::min(gy + 1, gridCount - 1);

                const float hL = gridHeights[gy * gridCount + xL];
                const float hR = gridHeights[gy * gridCount + xR];
                const float hD = gridHeights[yD * gridCount + gx];
                const float hU = gridHeights[yU * gridCount + gx];

                gridNormals[gy * gridCount + gx] = glm::normalize(
                    glm::vec3(hL - hR, hD - hU, 2.0f * gridStep));
            }
        }

        auto sampleGrid = [&](glm::vec2 worldXY,
                              const auto& gridValues) -> auto {
            const glm::vec2 coordF = (worldXY - gridOrigin) / gridStep;
            const int gx = std::clamp(static_cast<int>(std::floor(coordF.x)), 0,
                                      gridCount - 2);
            const int gy = std::clamp(static_cast<int>(std::floor(coordF.y)), 0,
                                      gridCount - 2);
            const glm::vec2 frac = coordF
                - glm::vec2(static_cast<float>(gx), static_cast<float>(gy));

            const auto& v00 = gridValues[gy * gridCount + gx];
            const auto& v10 = gridValues[gy * gridCount + gx + 1];
            const auto& v01 = gridValues[(gy + 1) * gridCount + gx];
            const auto& v11 = gridValues[(gy + 1) * gridCount + gx + 1];

            return glm::mix(glm::mix(v00, v10, frac.x),
                            glm::mix(v01, v11, frac.x), frac.y);
        };

        std::vector<glm::vec4> data(static_cast<size_t>(texSize) * texSize);
        std::vector<glm::vec3> normals(static_cast<size_t>(texSize) * texSize);
        std::vector<uint8_t> paintIndices(static_cast<size_t>(texSize)
                                          * texSize);

        for (uint32_t ty = 0; ty < texSize; ++ty)
        {
            for (uint32_t tx = 0; tx < texSize; ++tx)
            {
                const glm::vec2 worldXY = node.origin
                    + glm::vec2(static_cast<float>(tx), static_cast<float>(ty))
                        * texelSize;

                const float h = sampleGrid(worldXY, gridHeights);
                const glm::vec3 fixedNormal =
                    glm::normalize(sampleGrid(worldXY, gridNormals));

                const glm::vec3 autoWeights = map.GetMaterialWeightsAt(worldXY);

                data[ty * texSize + tx] =
                    glm::vec4(h, autoWeights.x, autoWeights.y, autoWeights.z);
                normals[ty * texSize + tx] = fixedNormal;
                paintIndices[ty * texSize + tx] = map.GetPaintIndexAt(worldXY);
            }
        }

        glTextureSubImage3D(
            heightNodeArray_, 0, 0, 0, static_cast<GLint>(layer),
            static_cast<GLsizei>(texSize), static_cast<GLsizei>(texSize), 1,
            GL_RGBA, GL_FLOAT, glm::value_ptr(data[0]));

        glTextureSubImage3D(
            normalNodeArray_, 0, 0, 0, static_cast<GLint>(layer),
            static_cast<GLsizei>(texSize), static_cast<GLsizei>(texSize), 1,
            GL_RGB, GL_FLOAT, glm::value_ptr(normals[0]));

        glTextureSubImage3D(paintNodeArray_, 0, 0, 0, static_cast<GLint>(layer),
                            static_cast<GLsizei>(texSize),
                            static_cast<GLsizei>(texSize), 1, GL_RED_INTEGER,
                            GL_UNSIGNED_BYTE, paintIndices.data());
    }

    void GLTerrainRenderer::render(const terrain::Map& map)
    {
        if (!Camera::get_active_camera())
        {
            Logger::LogError("Cant render terrain with no active camera!");
            return;
        }

        const uint64_t currentGeneration = map.GetGeneration();
        if (currentGeneration != lastSeenMapGeneration_)
        {
            lastSeenMapGeneration_ = currentGeneration;
            InvalidateAllNodes();
        }

        Camera* cam = Camera::get_active_camera();
        cam->calculate_matrices();

        const glm::vec3 cameraPos = cam->get_position();
        const glm::vec2 cameraXY(cameraPos.x, cameraPos.y);

        const float worldSize = map.GetWorldSizeMeters();
        const glm::vec2 worldMin(worldSize * -0.5f);
        const glm::vec2 worldMax(worldSize * 0.5f);

        const glm::vec2 heightRange = map.GetHeightRange();
        currentSkirtDropMeters_ =
            std::max((heightRange.y - heightRange.x) * kSkirtDropFraction,
                     kMinSkirtDropMeters);
        const float clampedSkirtAngle =
            std::clamp(kSkirtAngleDegrees, 5.0f, 85.0f);

        currentSkirtOutwardMeters_ =
            currentSkirtDropMeters_ / std::tan(glm::radians(clampedSkirtAngle));

        std::vector<QuadNode> leaves;
        BuildLeafSet(cameraXY, worldMin, worldMax, leaves);

        for (auto& slot : residentNodes_)
        {
            slot.second.inUseThisFrame = false;
        }

        struct DrawEntry
        {
            const QuadNode* node;
            uint32_t layer;
        };
        std::vector<DrawEntry> toDraw;
        toDraw.reserve(leaves.size());

        std::vector<const QuadNode*> needsAllocation;
        needsAllocation.reserve(leaves.size());

        for (const QuadNode& node : leaves)
        {
            const NodeKey key{ node.level, node.x, node.y };
            auto it = residentNodes_.find(key);
            if (it != residentNodes_.end())
            {
                it->second.inUseThisFrame = true;
                toDraw.push_back({ &node, it->second.layer });
            }
            else
            {
                needsAllocation.push_back(&node);
            }
        }

        for (auto it = residentNodes_.begin(); it != residentNodes_.end();)
        {
            if (!it->second.inUseThisFrame)
            {
                layerInUse_[it->second.layer] = false;
                it = residentNodes_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for (const QuadNode* nodePtr : needsAllocation)
        {
            uint32_t freeLayer = kMaxActiveNodes;
            for (uint32_t l = 0; l < layerInUse_.size(); ++l)
            {
                if (!layerInUse_[l])
                {
                    freeLayer = l;
                    break;
                }
            }

            if (freeLayer == kMaxActiveNodes)
            {
                Logger::LogError("GLTerrainRenderer: out of texture layers for "
                                 "quadtree nodes");
                continue;
            }

            const NodeKey key{ nodePtr->level, nodePtr->x, nodePtr->y };
            layerInUse_[freeLayer] = true;
            BuildNodeData(*nodePtr, map, freeLayer);
            residentNodes_[key] = ResidentNode{ freeLayer, true };
            toDraw.push_back({ nodePtr, freeLayer });
        }

        currentInstanceCount_ = toDraw.size();

        const GlShader* shader =
            dynamic_cast<const GlShader*>(terrainShader_.get());
        shader->use();

        shader->setValue("projection", cam->get_projection_matrix_());
        shader->setValue("view", cam->get_view_matrix_());
        shader->setValue("viewPos", cameraPos);
        shader->setValue("nodeTexelCount", static_cast<float>(kNodeResolution));
        shader->setValue("skirtDropMeters", currentSkirtDropMeters_);
        shader->setValue("skirtOutwardMeters", currentSkirtOutwardMeters_);

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
            5,
            dynamic_cast<GLShadowRenderer*>(ShadowRenderer::getInstance())
                ->depthMap);
        shader->setValue("shadowMap", 5);

        GL_Skybox* skybox = dynamic_cast<GL_Skybox*>(Skybox::getInstance());
        bool haveSkybox = skybox && skybox->is_loaded();
        shader->setValue("haveSkybox", haveSkybox);

        if (haveSkybox)
        {
            glBindTextureUnit(6, skybox->get_irradiance_map());
            shader->setValue("irradianceMap", 6);
            shader->setValue("ambientIntensity",
                             skybox->get_ambient_intensity());

            glBindTextureUnit(9, skybox->get_environment_map());
            shader->setValue("skyboxEnvironmentMap", 9);
            shader->setValue("skyboxExposure", skybox->get_exposure());
        }

        shader->setValue("fogDensity", graphics::FogSettings::density);
        shader->setValue("fogFallbackColor",
                         graphics::FogSettings::fallbackColor);

        glBindTextureUnit(0, heightNodeArray_);
        shader->setValue("heightNodeArray", 0);

        glBindTextureUnit(1, materialGaussianAlbedoArray_);
        shader->setValue("materialGaussianAlbedoArray", 1);

        glBindTextureUnit(2, materialAlbedoLutArray_);
        shader->setValue("materialAlbedoLutArray", 2);

        glBindTextureUnit(3, materialNormalArray_);
        shader->setValue("materialNormalArray", 3);

        glBindTextureUnit(4, materialOrmArray_);
        shader->setValue("materialOrmArray", 4);

        glBindTextureUnit(7, normalNodeArray_);
        shader->setValue("normalNodeArray", 7);

        glBindTextureUnit(8, paintNodeArray_);
        shader->setValue("paintNodeArray", 8);

        const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);

        if (currentInstanceCount_ > 0)
        {
            std::vector<glm::vec4> instanceData;
            instanceData.reserve(toDraw.size());
            for (const DrawEntry& entry : toDraw)
            {
                instanceData.emplace_back(
                    entry.node->origin.x, entry.node->origin.y,
                    entry.node->size, static_cast<float>(entry.layer));
            }

            glNamedBufferSubData(nodeInstanceBuffer_, 0,
                                 static_cast<GLsizeiptr>(instanceData.size()
                                                         * sizeof(glm::vec4)),
                                 instanceData.data());

            glBindVertexArray(nodeVao_);
            glDrawElementsInstanced(
                GL_TRIANGLES, static_cast<GLsizei>(nodeIndexCount_),
                GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instanceData.size()));
        }

        if (cullWasEnabled)
        {
            glEnable(GL_CULL_FACE);
        }
    }

    bool
    GLTerrainRenderer::CastsShadowOnCascade(size_t /*cascadeLayer*/,
                                            size_t totalCascadeLayers) const
    {
        return totalCascadeLayers > 0;
    }

    void GLTerrainRenderer::RenderShadow(size_t cascadeLayer) const
    {
        if (currentInstanceCount_ == 0)
        {
            return;
        }

        const GlShader* shader =
            dynamic_cast<const GlShader*>(terrainShadowShader_.get());
        shader->use();

        GLShadowRenderer::invalidate_active_shadow_shader();

        shader->setValue(
            "lightSpaceMatrix",
            GLShadowRenderer::get_cascade_light_matrix(cascadeLayer));
        shader->setValue("nodeTexelCount", static_cast<float>(kNodeResolution));
        shader->setValue("skirtDropMeters", currentSkirtDropMeters_);
        shader->setValue("skirtOutwardMeters", currentSkirtOutwardMeters_);

        glBindTextureUnit(0, heightNodeArray_);
        shader->setValue("heightNodeArray", 0);

        const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);

        glBindVertexArray(nodeVao_);
        glDrawElementsInstanced(
            GL_TRIANGLES, static_cast<GLsizei>(nodeIndexCount_),
            GL_UNSIGNED_INT, 0, static_cast<GLsizei>(currentInstanceCount_));

        if (cullWasEnabled)
        {
            glEnable(GL_CULL_FACE);
        }
    }
} // namespace raphEngine::graphics::ogl
