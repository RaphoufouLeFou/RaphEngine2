#include "graphics/mesh_buffers.hpp"

#include <RaphEngine2/export.hpp>
#include <memory>

#include "RaphEngine2/graphics/ogl/gl_mesh_buffers.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"
#include "logger/logger.hpp"

namespace raphEngine::graphics
{
    std::unique_ptr<MeshBuffers>
    MeshBuffers::getMeshBuffer(raphEngine::objects::Mesh* mesh)
    {
        if (Settings::Get<GraphicsSettings>().api == "OpenGL")
        {
            return std::make_unique<GLMeshBuffers>(mesh);
        }
        if (Settings::Get<GraphicsSettings>().api == "Vulkan")
        {
            Logger::LogError("Cannot get mesh buffer from Vulkan",
                             " (Not implemented) ", "Defaulting to OpenGl");
            return std::make_unique<GLMeshBuffers>(mesh);
            // TODO: for later
        }
        if (Settings::Get<GraphicsSettings>().api == "D3D11")
        {
            Logger::LogError("Cannot get mesh buffer from DirectX 11",
                             " (Not implemented) ", "Defaulting to OpenGl");
            return std::make_unique<GLMeshBuffers>(mesh);
            // TODO: for later
        }

        Logger::LogError("Cannot get mesh buffer from an unknown grpahics API",
                         " Defaulting to OpenGl");

        return std::make_unique<GLMeshBuffers>(mesh);
    }

} // namespace raphEngine::graphics
