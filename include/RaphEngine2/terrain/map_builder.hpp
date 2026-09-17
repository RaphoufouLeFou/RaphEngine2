#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/chunk.hpp>
#include <RaphEngine2/terrain/erosion.hpp>
#include <RaphEngine2/terrain/material_generator.hpp>
#include <RaphEngine2/terrain/noise.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{

    RAPHENGINE_API void BuildMapFromHeightmap(
        const fs::path& heightmapPath, float sizeInMeters, float height,
        const fs::path& outputDirectory,
        const std::optional<HydraulicErosionParams>& erosionParams =
            std::nullopt);

    RAPHENGINE_API void BuildMapFromNoise(
        const FractalNoiseParams& noiseParams, float sizeInMeters, float height,
        const fs::path& outputDirectory,
        const std::optional<HydraulicErosionParams>& erosionParams =
            std::nullopt);

    RAPHENGINE_API void
    BuildMapShellFromNoise(const FractalNoiseParams& noiseParams,
                           float sizeInMeters, float height,
                           const fs::path& outputDirectory);

    RAPHENGINE_API void
    BuildMapShellFromHeightmap(const fs::path& heightmapPath,
                               float sizeInMeters, float height,
                               const fs::path& outputDirectory);

    class RAPHENGINE_API NoiseChunkGenerator
    {
    public:
        NoiseChunkGenerator(const FractalNoiseParams& params, float height);

        void SetParams(const FractalNoiseParams& params, float height);

        void operator()(glm::ivec2 gridCoord, const fs::path& chunkPath) const;

    private:
        PerlinNoise noise_;
        MaterialGenerator materialGenerator_;
        FractalNoiseParams params_;
        float height_;
    };

    class RAPHENGINE_API ImageChunkGenerator
    {
    public:
        ImageChunkGenerator(const fs::path& heightmapPath, float sizeInMeters,
                            float height);
        ~ImageChunkGenerator();

        ImageChunkGenerator(const ImageChunkGenerator&) = delete;
        ImageChunkGenerator& operator=(const ImageChunkGenerator&) = delete;
        ImageChunkGenerator(ImageChunkGenerator&&) = delete;
        ImageChunkGenerator& operator=(ImageChunkGenerator&&) = delete;

        void operator()(glm::ivec2 gridCoord, const fs::path& chunkPath) const;

    private:
        uint16_t* sourcePixels_ = nullptr;
        int sourceWidth_ = 0;
        int sourceHeight_ = 0;
        uint32_t fullResolution_ = 0;
        float height_ = 0.0f;
        MaterialGenerator materialGenerator_;
    };
} // namespace raphEngine::terrain
