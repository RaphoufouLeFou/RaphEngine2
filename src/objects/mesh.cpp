#include "RaphEngine2/objects/mesh.hpp"

namespace raphEngine::objects
{
    const std::vector<Vertex>& Mesh::get_vertices() const
    {
        return vertices_;
    }
}