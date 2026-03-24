#include "objects/mesh.hpp"
#include <RaphEngine2/graphics/mesh_renderer.hpp>

namespace raphEngine::objects
{
    Mesh::Mesh()
    {
        mesh_renderer_ = graphics::MeshRenderer::createMeshRender(this);
    }

    const std::vector<Vertex>& Mesh::get_vertices() const
    {
        return vertices_;
    }
} // namespace raphEngine::objects
