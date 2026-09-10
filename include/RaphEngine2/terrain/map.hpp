#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/chunk.hpp>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

namespace raphEngine::terrain
{
    class RAPHENGINE_API Map
    {
    public:

        void Load(const fs::path&);
        void Save(const fs::path&);

    private:
	std::vector<std::vector<std::unqiue_ptr<Chunk>>> chunks;

    };
} // namespace raphEngine
