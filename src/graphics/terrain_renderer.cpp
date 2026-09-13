// terrain_renderer.cpp
#include "graphics/terrain_renderer.hpp"

#include <RaphEngine2/export.hpp>
#include <memory>

#include "graphics/ogl/gl_terrain_renderer.hpp"
#include "logger/logger.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"

namespace raphEngine::graphics
{
    std::unique_ptr<TerrainRenderer> TerrainRenderer::instance_ = nullptr;

    TerrainRenderer* TerrainRenderer::getInstance()
    {
        if (instance_ == nullptr)
        {
            if (Settings::Get<GraphicsSettings>().api == "OpenGL")
            {
                instance_ = std::make_unique<ogl::GLTerrainRenderer>();
                return instance_.get();
            }
            if (Settings::Get<GraphicsSettings>().api == "Vulkan")
            {
                Logger::LogError("Cannot get terrain renderer from Vulkan",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<ogl::GLTerrainRenderer>();
                return instance_.get();
            }
            if (Settings::Get<GraphicsSettings>().api == "D3D11")
            {
                Logger::LogError("Cannot get terrain renderer from DirectX 11",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<ogl::GLTerrainRenderer>();
                return instance_.get();
            }

            Logger::LogError(
                "Cannot get terrain renderer from an unknown graphics API.",
                " Defaulting to OpenGl");

            instance_ = std::make_unique<ogl::GLTerrainRenderer>();
        }

        return instance_.get();
    }
} // namespace raphEngine::graphics
