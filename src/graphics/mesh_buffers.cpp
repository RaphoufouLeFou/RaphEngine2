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
    std::unordered_map<MeshSourceKey, std::weak_ptr<MeshBuffers>,
                       MeshSourceKeyHash>
        MeshBuffers::cache_;

    std::shared_ptr<MeshBuffers>
    MeshBuffers::CreateBackendBuffers(raphEngine::objects::Mesh* mesh)
    {
        if (Settings::Get<GraphicsSettings>().api == "OpenGL")
            return std::make_shared<GLMeshBuffers>(mesh);
        if (Settings::Get<GraphicsSettings>().api == "Vulkan")
        {
            Logger::LogError("Cannot get mesh buffer from Vulkan",
                             " (Not implemented). ", "Defaulting to OpenGl");
            return std::make_shared<GLMeshBuffers>(mesh);
        }
        if (Settings::Get<GraphicsSettings>().api == "D3D11")
        {
            Logger::LogError("Cannot get mesh buffer from DirectX 11",
                             " (Not implemented). ", "Defaulting to OpenGl");
            return std::make_shared<GLMeshBuffers>(mesh);
        }
        Logger::LogError("Cannot get mesh buffer from an unknown grpahics API.",
                         " Defaulting to OpenGl");
        return std::make_shared<GLMeshBuffers>(mesh);
    }

    std::shared_ptr<MeshBuffers>
    MeshBuffers::getMeshBuffer(raphEngine::objects::Mesh* mesh)
    {
        MeshSourceKey key = mesh->get_source_key();

        if (key.resource != nullptr)
        {
            if (auto it = cache_.find(key); it != cache_.end())
                if (auto locked = it->second.lock())
                    return locked;
        }

        std::shared_ptr<MeshBuffers> buffers = CreateBackendBuffers(mesh);

        if (key.resource != nullptr)
            cache_[key] = buffers;

        return buffers;
    }
} // namespace raphEngine::graphics