#pragma once

#include <glm/glm.hpp>

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include "mesh.hpp"
#include "mesh_info.hpp"
#include <memory>

namespace raphEngine::objects
{
    class RAPHENGINE_API ObjectMesh
    {
    public:
        ObjectMesh(const MeshInfo& mesh_info);
        
    private:
        std::vector<std::unique_ptr<Mesh>> meshes_;
    };
} // namespace raphEngine
