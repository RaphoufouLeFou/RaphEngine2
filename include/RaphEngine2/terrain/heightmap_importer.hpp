#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/chunk.hpp>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{

    RAPHENGINE_API void ImportHeightmapAsChunk(const fs::path& imagePath,
                                               glm::ivec2 gridCoord,
                                               glm::vec2 worldHeightRange,
                                               const fs::path& outputChunkPath);
} // namespace raphEngine::terrain
