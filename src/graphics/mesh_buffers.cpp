#include "graphics/mesh_buffers.hpp"

#include <RaphEngine2/export.hpp>
#include <memory>

#include "RaphEngine2/graphics/ogl/gl_mesh_buffers.hpp"
#include "RaphEngine2/objects/mesh.hpp"

namespace raphEngine::graphics
{
    std::unique_ptr<MeshBuffers> MeshBuffers::getMeshBuffer(raphEngine::objects::Mesh* mesh)
    {
        // TODO: select the correct API
        return std::make_unique<GLMeshBuffers>(mesh);
    }

} // namespace raphEngine::graphics
