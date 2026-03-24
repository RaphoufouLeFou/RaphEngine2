#include "objects/mesh.hpp"
#include "graphics/mesh_renderer.hpp"

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

    std::vector<Vertex>& Mesh::get_vertices()
    {
        return vertices_;
    }

    const std::vector<unsigned int>& Mesh::get_indices() const
    {
        return indices_;
    }

    std::vector<unsigned int>& Mesh::get_indices()
    {
        return indices_;
    }

    const std::vector<Texture>& Mesh::get_textures() const
    {
        return textures_;
    }

    std::vector<Texture>& Mesh::get_textures()
    {
        return textures_;
    }

    void Mesh::set_model_matrix(const glm::mat4& model_matrix)
    {
        model_matrix_ = model_matrix;
    }

    void Mesh::render()
    {
        mesh_renderer_->render();
    }


} // namespace raphEngine::objects
