#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include <RaphEngine2/graphics/mesh_renderer.hpp>

namespace raphEngine::graphics
{
    class MeshRenderer;
}

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
        Vertex(glm::vec3 position_) : position(position_) {}
    };

    struct RAPHENGINE_API Texture
    {
        enum TextureType
        {
            DIFFUSE,
            SPECULAR,
            NORMAL,
            HEIGHT,
        };

        unsigned int id;
        TextureType type;
        std::string path;
    };

    class RAPHENGINE_API Mesh
    {
    public:
        Mesh();
        virtual ~Mesh() = default;

        virtual void render();

        const std::vector<Vertex>& get_vertices() const;
        std::vector<Vertex>& get_vertices();

        const std::vector<unsigned int>& get_indices() const;
        std::vector<unsigned int>& get_indices();

        const std::vector<Texture>& get_textures() const;
        std::vector<Texture>& get_textures();

        void set_model_matrix(const glm::mat4& model_matrix);

    protected:
        std::vector<Vertex> vertices_;
        std::vector<unsigned int> indices_;
        std::vector<Texture> textures_;

        bool outline_;
        bool cast_shadows_;

        glm::mat4 model_matrix_;

        std::shared_ptr<graphics::MeshRenderer> mesh_renderer_;
    };
} // namespace raphEngine
