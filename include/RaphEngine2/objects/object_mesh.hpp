#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>

#include "mesh.hpp"
#include "mesh_info.hpp"
#include "renderable.hpp"
#include "resources/model_resource.hpp"
#include "graphics/shader.hpp"

namespace raphEngine::objects
{
    class RAPHENGINE_API ObjectMesh
    {
    public:
        ObjectMesh() = default;
        ObjectMesh(const MeshInfo& info, graphics::Shader* shader);

        static std::vector<Texture> textures_loaded_;
        void add_mesh(std::unique_ptr<Mesh> mesh);

    private:
        resources::Resource* meshes_resource_;
        graphics::Shader* shader_;
    };
} // namespace raphEngine::objects
