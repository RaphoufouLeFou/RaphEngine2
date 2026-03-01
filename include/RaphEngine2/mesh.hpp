#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "export.hpp"

namespace raphEngine
{
    struct RAPHENGINE_API Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 tex_coords;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    struct RAPHENGINE_API Texture
    {
        unsigned int id;
        std::string type;
        std::string path;
    };

    class RAPHENGINE_API Mesh
    {
    private:
        std::vector<Vertex> vertices_;
        std::vector<unsigned int> indices_;
        std::vector<Texture> textures_;
    };
} // namespace raphEngine
