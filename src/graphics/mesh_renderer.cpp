#include "graphics/mesh_renderer.hpp"

#include <RaphEngine2/export.hpp>
#include <memory>

#include "RaphEngine2/graphics/ogl/gl_mesh_renderer.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"
#include "logger/logger.hpp"

namespace raphEngine::graphics
{

    std::unique_ptr<MeshRenderer> MeshRenderer::instance_ = nullptr;

    MeshRenderer* MeshRenderer::getInstance()
    {
        if (instance_ == nullptr)
        {
            if (Settings::Get<GraphicsSettings>().api == "OpenGL")
            {
                instance_ = std::make_unique<GLMeshRenderer>();
                return instance_.get();
            }
            if (Settings::Get<GraphicsSettings>().api == "Vulkan")
            {
                Logger::LogError("Cannot get mesh renderer from Vulkan",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<GLMeshRenderer>();
                return instance_.get();
                // TODO: for later
            }
            if (Settings::Get<GraphicsSettings>().api == "D3D11")
            {
                Logger::LogError("Cannot get mesh renderer from DirectX 11",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<GLMeshRenderer>();
                return instance_.get();
                // TODO: for later
            }

            Logger::LogError(
                "Cannot get mesh renderer from an unknown grpahics API.",
                " Defaulting to OpenGl");

            instance_ = std::make_unique<GLMeshRenderer>();
        }

        return instance_.get();
    }
} // namespace raphEngine::graphics
