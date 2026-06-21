#include "objects/mesh.hpp"

#include "graphics/mesh_renderer.hpp"

namespace raphEngine::objects
{
    Mesh::Mesh()
    {
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
    
    const glm::mat4& Mesh::get_model_matrix() const
    {
        return model_matrix_;
    }

    void Mesh::set_shader(graphics::Shader* shader)
    {
        shader_ = shader;
    }

    const graphics::Shader* Mesh::get_shader() const
    {
        return shader_;
    }

    void Mesh::generate_mesh_buffers()
    {
        buffers_ = graphics::MeshBuffers::getMeshBuffer(this);
    }

    void Mesh::render() const
    {
        graphics::MeshRenderer::getInstance()->render(this);
    }

} // namespace raphEngine::objects
