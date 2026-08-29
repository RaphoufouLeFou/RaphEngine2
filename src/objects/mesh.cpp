#include "objects/mesh.hpp"
#include <algorithm>
#include <cstring>

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
        return data_->vertices;
    }

    std::vector<Vertex>& Mesh::get_vertices()
    {
        return data_->vertices;
    }

    const std::vector<unsigned int>& Mesh::get_indices() const
    {
        return data_->indices;
    }

    std::vector<unsigned int>& Mesh::get_indices()
    {
        return data_->indices;
    }

    const std::vector<Texture>& Mesh::get_textures() const
    {
        return data_->textures;
    }

    std::vector<Texture>& Mesh::get_textures()
    {
        return data_->textures;
    }

    bool Mesh::get_outline() const
    {
        if (!outline_)
            return false;
        return *outline_;
    }

    void Mesh::set_outline(bool* outline)
    {
        outline_ = outline;
    }

    void Mesh::set_model_matrix(const glm::mat4& model_matrix)
    {
        data_->local_matrix = model_matrix;
    }

    const glm::mat4& Mesh::get_model_matrix() const
    {
        return data_->local_matrix;
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

        auto foldFloat = [&h](float f) {
            uint32_t bits;
            std::memcpy(&bits, &f, sizeof(bits));
            h = h * 1099511628211ull ^ static_cast<uint64_t>(bits);
        };

        foldFloat(data_->metallic_factor);
        foldFloat(data_->roughness_factor);
        foldFloat(data_->emissive_factor.x);
        foldFloat(data_->emissive_factor.y);
        foldFloat(data_->emissive_factor.z);
        foldFloat(data_->alpha_cutoff);
        h = h * 1099511628211ull ^ static_cast<uint64_t>(data_->alpha_mask);
        h = h * 1099511628211ull
            ^ static_cast<uint64_t>(data_->metallic_roughness_packed);

        return h;
    }

    glm::vec3 Mesh::get_lower_bounds() const
    {
        return data_->bounds_min;
    }

    glm::vec3 Mesh::get_higher_bounds() const
    {
        return data_->bounds_max;
    }

    void Mesh::get_world_bounds(glm::vec3& out_min, glm::vec3& out_max) const
    {
        const glm::vec3& lo = data_->bounds_min;
        const glm::vec3& hi = data_->bounds_max;
        const glm::mat4 world =
            parent_object->get_transform().get_model_matrix()
            * get_model_matrix();

        const glm::vec3 corners[8] = {
            { lo.x, lo.y, lo.z }, { hi.x, lo.y, lo.z }, { lo.x, hi.y, lo.z },
            { lo.x, lo.y, hi.z }, { hi.x, hi.y, lo.z }, { hi.x, lo.y, hi.z },
            { lo.x, hi.y, hi.z }, { hi.x, hi.y, hi.z },
        };

        out_min = glm::vec3(std::numeric_limits<float>::max());
        out_max = glm::vec3(std::numeric_limits<float>::lowest());
        for (const auto& c : corners)
        {
            glm::vec3 wc = glm::vec3(world * glm::vec4(c, 1.0f));
            out_min = glm::min(out_min, wc);
            out_max = glm::max(out_max, wc);
        }
    }

    void Mesh::get_world_sphere(glm::vec3& out_center, float& out_radius) const
    {
        const glm::mat4 world =
            parent_object->get_transform().get_model_matrix()
            * get_model_matrix();

        out_center =
            glm::vec3(world * glm::vec4(data_->local_sphere_center, 1.0f));

        // Conservative radius scale — max basis-vector length handles
        // uniform and non-uniform scale without decomposing the matrix.
        float sx = glm::length(glm::vec3(world[0]));
        float sy = glm::length(glm::vec3(world[1]));
        float sz = glm::length(glm::vec3(world[2]));
        float max_scale = std::max({ sx, sy, sz });

        out_radius = data_->local_sphere_radius * max_scale;
    }
} // namespace raphEngine::objects
