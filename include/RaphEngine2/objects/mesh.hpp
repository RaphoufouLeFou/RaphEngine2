#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include "shader.hpp"

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
        Vertex(glm::vec3 position_) : position {position_} {}
    };

    struct RAPHENGINE_API Texture
    {
        unsigned int id;
        std::string type;
        std::string path;
    };

    class RAPHENGINE_API Mesh
    {
    public:
        virtual void render() = 0;
        const std::vector<Vertex>& get_vertices() const;
    protected:
        std::vector<Vertex> vertices_;
        std::vector<unsigned int> indices_;
        std::vector<Texture> textures_;

        bool outline_;
        bool cast_shadows_;
    };
} // namespace raphEngine
