#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>

#include "RaphEngine2/graphics/shader.hpp"
#include "RaphEngine2/renderable.hpp"
#include "RaphEngine2/resources/model_resource.hpp"
#include "mesh.hpp"
#include "mesh_info.hpp"

namespace raphEngine::objects
{
    class RAPHENGINE_API ObjectMesh
    {
    public:
        ObjectMesh() = default;
        ObjectMesh(objects::GameObject* parent_object, const MeshInfo& info,
                   graphics::Shader* shader);

        static std::vector<Texture> textures_loaded_;
        void add_mesh(std::unique_ptr<Mesh> mesh);

        void render() const;
        std::vector<std::unique_ptr<objects::Mesh>> meshes_;

    private:
        // TODO: replace the vec by the recource system
        // resources::Resource* meshes_resource_;

        graphics::Shader* shader_;
        objects::GameObject* parent_object;
    };
} // namespace raphEngine::objects
