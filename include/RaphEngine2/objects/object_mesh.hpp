#pragma once

#include <glm/glm.hpp>

#include <RaphEngine2/export.hpp>
#include "renderable.hpp"
#include "mesh.hpp"
#include "mesh_info.hpp"
#include <memory>

namespace raphEngine::objects
{
    class RAPHENGINE_API ObjectMesh
    {
    public:
    
        ObjectMesh() = default;
        ObjectMesh(const MeshInfo& info);
    
        ObjectMesh(const ObjectMesh&) = delete;
        ObjectMesh& operator=(const ObjectMesh&) = delete;
        ObjectMesh& operator=(ObjectMesh&&) = default;
        
        static std::vector<Texture> textures_loaded_;
        void add_mesh(std::unique_ptr<Mesh> mesh);

    private:
        std::shared_ptr<Shader> shader_;
        std::vector<std::unique_ptr<Mesh>> meshes_;
    };
} // namespace raphEngine
