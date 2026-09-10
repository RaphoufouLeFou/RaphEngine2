#pragma once

#include <RaphEngine2/RaphEngine2.hpp>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    class RAPHENGINE_API Chunk
    {
    public:

        static void Load(const fs::path&);
        static void Save(const fs::path&);
    };
} // namespace raphEngine
