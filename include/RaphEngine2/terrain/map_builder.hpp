#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/chunk.hpp>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    RAPHENGINE_API void BuildMapFromHeightmap(const fs::path& heightmapPath,
                                              float sizeInMeters, float height,
                                              const fs::path& outputDirectory);
} // namespace raphEngine::terrain
