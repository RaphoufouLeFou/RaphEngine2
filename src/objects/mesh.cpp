#include "objects/mesh.hpp"

#include "graphics/mesh_renderer.hpp"
#include "graphics/shadow_renderer.hpp"
#include "resources/model_resource.hpp" // complete type needed for ~Mesh()

namespace raphEngine::objects
{
    Mesh::Mesh()
    {}
    Mesh::~Mesh() = default;

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

    const graphics::MeshBuffers* Mesh::get_buffers() const
    {
        return buffers_.get();
    }

    void Mesh::generate_mesh_buffers()
    {
        buffers_ = graphics::MeshBuffers::getMeshBuffer(this);
        material_hash_ = compute_material_hash();
    }

    void Mesh::set_source(std::shared_ptr<resources::ModelResource> model,
                          size_t submesh_index)
    {
        source_model_ = std::move(model);
        submesh_index_ = submesh_index;
    }

    graphics::MeshSourceKey Mesh::get_source_key() const
    {
        return { source_model_.get(), submesh_index_ };
    }
    void Mesh::render() const
    {
        graphics::MeshRenderer::getInstance()->render(this);
    }

    void Mesh::render_shadow() const
    {
        graphics::ShadowRenderer::getInstance()->render_shadows(this);
    }

    BatchKey Mesh::get_batch_key() const
    {
        return { buffers_.get(), shader_, material_hash_ };
    }

    uint64_t Mesh::compute_material_hash() const
    {
        uint64_t h = 0;
        for (auto& tex : get_textures())
            h = h * 1099511628211ull ^ static_cast<uint64_t>(tex.id);
        return h;
    }

    glm::vec3 Mesh::get_lower_bounds() const
    {
        if (vertices_.size() == 0)
        {
            return glm::vec3{ 0 };
        }

        glm::vec3 lower_bounds = vertices_.at(0).position;

        for (const auto& v : vertices_)
        {
            if (lower_bounds.x > v.position.x)
                lower_bounds.x = v.position.x;
            if (lower_bounds.y > v.position.y)
                lower_bounds.y = v.position.y;
            if (lower_bounds.z > v.position.z)
                lower_bounds.z = v.position.z;
        }

        return lower_bounds;
    }

    glm::vec3 Mesh::get_higher_bounds() const
    {
        if (vertices_.size() == 0)
        {
            return glm::vec3{ 0 };
        }

        glm::vec3 higher_bounds = vertices_.at(0).position;

        for (const auto& v : vertices_)
        {
            if (higher_bounds.x > v.position.x)
                higher_bounds.x = v.position.x;
            if (higher_bounds.y > v.position.y)
                higher_bounds.y = v.position.y;
            if (higher_bounds.z > v.position.z)
                higher_bounds.z = v.position.z;
        }

        return higher_bounds;
    }
} // namespace raphEngine::objects