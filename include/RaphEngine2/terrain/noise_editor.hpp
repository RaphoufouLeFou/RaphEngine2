#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/terrain/map_builder.hpp>
#include <RaphEngine2/terrain/noise.hpp>

#include <filesystem>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    RAPHENGINE_API void
    DrawNoiseEditorWindow(FractalNoiseParams& params, float sizeInMeters,
                          float height, const fs::path& outputDirectory,
                          NoiseChunkGenerator* liveGenerator,
                          bool* open = nullptr);
} // namespace raphEngine::terrain
