#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "RaphEngine2/objects/mesh.hpp"

namespace raphEngine::resources
{
    struct RAPHENGINE_API SubmeshData
    {
        std::vector<objects::Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<objects::Texture> textures;
        glm::mat4 local_matrix;
        glm::vec3 bounds_min{ 0.0f };
        glm::vec3 bounds_max{ 0.0f };
        glm::vec3 local_sphere_center{ 0.0f };
        float local_sphere_radius = 0.0f;
    };

    class RAPHENGINE_API ModelResource
    {
    public:
        static std::shared_ptr<ModelResource>
        get_or_load(const std::string& path, bool filter);

        std::vector<SubmeshData>& get_submeshes()
        {
            return submeshes_;
        }

        ModelResource(const ModelResource&) = delete;
        ModelResource& operator=(const ModelResource&) = delete;

    private:
        ModelResource(const std::string& path, bool filter);

        std::vector<SubmeshData> submeshes_;

        static std::unordered_map<std::string, std::weak_ptr<ModelResource>>
            cache_;
    };
} // namespace raphEngine::resources
