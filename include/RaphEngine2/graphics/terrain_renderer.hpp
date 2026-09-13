// terrain_renderer.hpp
#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>

namespace raphEngine::terrain
{
    class Map;
}

namespace raphEngine::graphics
{
    class RAPHENGINE_API TerrainRenderer
    {
    public:
        virtual ~TerrainRenderer() = default;

        virtual void render(const terrain::Map& map) = 0;

        static TerrainRenderer* getInstance();

    private:
        static std::unique_ptr<TerrainRenderer> instance_;
    };
} // namespace raphEngine::graphics
