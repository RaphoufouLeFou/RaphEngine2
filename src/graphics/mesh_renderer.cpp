#include <RaphEngine2/export.hpp>
#include <memory>
#include "objects/mesh.hpp"
#include "graphics/mesh_renderer.hpp"
#include "graphics/ogl/gl_mesh_renderer.hpp"
#include "settings/graphics.hpp"

namespace raphEngine::graphics
{

    std::shared_ptr<MeshRenderer> MeshRenderer::createMeshRender(objects::Mesh * mesh)
    {
        return std::make_shared<GLMeshRenderer>(mesh);
    }
}
