#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>

#include "mesh.hpp"
#include "mesh_info.hpp"
#include "renderable.hpp"

namespace raphEngine::objects
{
    class RAPHENGINE_API ObjectMesh
    {
    public:
        ObjectMesh() = default;
        ObjectMesh(const MeshInfo& info);

        static std::vector<Texture> textures_loaded_;
        void add_mesh(std::unique_ptr<Mesh> mesh);

    private:
        std::shared_ptr<Shader> shader_;
        std::vector<std::unique_ptr<Mesh>> meshes_;
    };
} // namespace raphEngine::objects
