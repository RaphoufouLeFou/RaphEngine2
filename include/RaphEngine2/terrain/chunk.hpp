#pragma once

#include <RaphEngine2/RaphEngine2.hpp>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    class RAPHENGINE_API Chunk
    {
    public:

        void Load(const fs::path&);
        void Save(const fs::path&);
    };
} // namespace raphEngine
