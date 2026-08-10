#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "objects/mesh.hpp" // for objects::Vertex, objects::Texture

namespace raphEngine::resources
{
    struct RAPHENGINE_API SubmeshData
    {
        std::vector<objects::Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<objects::Texture> textures;
        glm::mat4 local_matrix;
    };

    class RAPHENGINE_API ModelResource
    {
    public:
        static std::shared_ptr<ModelResource>
        get_or_load(const std::string& path, bool filter);

        const std::vector<SubmeshData>& get_submeshes() const
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