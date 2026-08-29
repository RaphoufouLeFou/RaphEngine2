#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/graphics/mesh_buffers.hpp>
#include <RaphEngine2/graphics/shader.hpp>
#include <RaphEngine2/objects/game_object.hpp>
#include <RaphEngine2/renderable.hpp>
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace raphEngine::graphics
{
    class MeshRenderer;
    class MeshBuffers;
} // namespace raphEngine::graphics

namespace raphEngine::resources
{
    struct SubmeshData;
    class ModelResource;
} // namespace raphEngine::resources

namespace raphEngine::objects
{
    struct RAPHENGINE_API Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 tex_coords;
        glm::vec3 tangent;
        glm::vec3 bitangent;

        Vertex() = default;
        Vertex(glm::vec3 position_)
            : position(position_)
        {}
    };

    struct RAPHENGINE_API Texture
    {
        enum TextureType
        {
            DIFFUSE,
            SPECULAR,
            NORMAL,
            HEIGHT,
            METALLIC,
            ROUGHNESS,
            AO,
            EMISSIVE,
            OPACITY
        };
        unsigned int id;
        TextureType type;
        std::string path;
        bool bilinear;
    };

    struct BatchKey
    {
        const graphics::MeshBuffers* buffers;
        const graphics::Shader* shader;
        uint64_t material_hash;
        bool operator==(const BatchKey&) const = default;
    };

    struct BatchKeyHash
    {
        size_t operator()(const BatchKey& k) const
        {
            size_t h = std::hash<const void*>{}(k.buffers);
            h ^= std::hash<const void*>{}(k.shader) + 0x9e3779b9 + (h << 6)
                + (h >> 2);
            h ^= std::hash<uint64_t>{}(k.material_hash) + 0x9e3779b9 + (h << 6)
                + (h >> 2);
            return h;
        }
    };

    class RAPHENGINE_API Mesh : public Renderable
    {
    public:
        Mesh();
        virtual ~Mesh(); // defined in mesh.cpp — needs ModelResource complete

        virtual void render() const override;
        virtual void render_shadow() const override;
        const objects::Mesh* as_mesh() const override
        {
            return this;
        }
        BatchKey get_batch_key() const;
        uint64_t compute_material_hash() const;

        const std::vector<Vertex>& get_vertices() const;
        std::vector<Vertex>& get_vertices();
        const std::vector<unsigned int>& get_indices() const;
        std::vector<unsigned int>& get_indices();
        const std::vector<Texture>& get_textures() const;
        std::vector<Texture>& get_textures();
        bool get_outline() const;
        void set_outline(bool* outline);

        void set_model_matrix(const glm::mat4& model_matrix);
        const glm::mat4& get_model_matrix() const;
        void set_shader(graphics::Shader* shader);
        const graphics::Shader* get_shader() const;
        const graphics::MeshBuffers* get_buffers() const;
        void generate_mesh_buffers();

        void set_source(std::shared_ptr<resources::ModelResource> model,
                        size_t submesh_index);
        graphics::MeshSourceKey get_source_key() const;

        glm::vec3 get_lower_bounds() const;
        glm::vec3 get_higher_bounds() const;
        void get_world_bounds(glm::vec3& out_min, glm::vec3& out_max) const;
        void get_world_sphere(glm::vec3& out_center, float& out_radius) const;

        objects::GameObject* parent_object;
        // glm::mat4 model_matrix_;
        resources::SubmeshData* data_;
        const bool* cast_shadows;

    protected:
        /*
            std::vector<Vertex> vertices_;
            std::vector<unsigned int> indices_;
            std::vector<Texture> textures_;
        */

        bool* outline_;
        std::shared_ptr<graphics::MeshBuffers> buffers_;
        graphics::Shader* shader_;

    private:
        uint64_t material_hash_;
        std::shared_ptr<resources::ModelResource> source_model_;
        size_t submesh_index_ = 0;
    };
} // namespace raphEngine::objects
