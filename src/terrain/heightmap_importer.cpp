#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/heightmap_importer.hpp>
#include <RaphEngine2/terrain/chunk_format.hpp>
#include <RaphEngine2/terrain/material_generator.hpp>

#include <stdexcept>

#include <stb_image.h>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    void ImportHeightmapAsChunk(const fs::path& imagePath, glm::ivec2 gridCoord,
                                glm::vec2 worldHeightRange,
                                const fs::path& outputChunkPath)
    {
        int width = 0, height = 0, channels = 0;
        uint16_t* pixels = stbi_load_16(imagePath.string().c_str(), &width,
                                        &height, &channels, 1);

        if (pixels == nullptr)
        {
            throw std::runtime_error("ImportHeightmapAsChunk: failed to load "
                                     + imagePath.string());
        }

        detail::FlipHeightRowsInPlace(pixels, static_cast<uint32_t>(width),
                                      static_cast<uint32_t>(height));

        if (static_cast<uint32_t>(width) != kChunkResolution
            || static_cast<uint32_t>(height) != kChunkResolution)
        {
            stbi_image_free(pixels);
            throw std::runtime_error("ImportHeightmapAsChunk: "
                                     + imagePath.string() + " must be exactly "
                                     + std::to_string(kChunkResolution) + "x"
                                     + std::to_string(kChunkResolution)
                                     + " (resampling isn't implemented here — "
                                       "see BuildMapFromHeightmap)");
        }

        try
        {
            // Same fixed seed BuildMapFromHeightmap/ImageChunkGenerator use
            // for single-image import -- there's no natural noise seed to
            // derive one from here, unlike the procedural-noise path.
            constexpr uint32_t kDefaultImageMaterialSeed = 918273645u;
            const MaterialGenerator materialGenerator(kDefaultImageMaterialSeed,
                                                      worldHeightRange);

            std::vector<float> heightsMeters(
                static_cast<size_t>(kChunkResolution) * kChunkResolution);
            for (size_t i = 0; i < heightsMeters.size(); ++i)
            {
                heightsMeters[i] =
                    glm::mix(worldHeightRange.x, worldHeightRange.y,
                             static_cast<float>(pixels[i]) / 65535.0f);
            }

            const glm::vec2 worldOrigin(
                static_cast<float>(gridCoord.x) * (kChunkResolution - 1),
                static_cast<float>(gridCoord.y) * (kChunkResolution - 1));
            const std::vector<MaterialWeights> materialWeights =
                materialGenerator.ComputeChunkWeights(
                    heightsMeters, kChunkResolution, worldOrigin);

            detail::WriteChunkFile(outputChunkPath, gridCoord,
                                   ChunkSource::Imported, worldHeightRange,
                                   pixels, materialWeights.data());
        }
        catch (...)
        {
            stbi_image_free(pixels);
            throw;
        }

        stbi_image_free(pixels);
    }
} // namespace raphEngine::terrain
