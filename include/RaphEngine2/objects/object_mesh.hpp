#pragma once

#include <glm/glm.hpp>

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include "mesh_loader.hpp"
#include "mesh.hpp"
#include "mesh_info.hpp"
#include <memory>

namespace raphEngine::objects
{
    class RAPHENGINE_API ObjectMesh
    {
    private:
        ObjectMesh();
        std::shared_ptr<Shader> shader_;
        std::vector<std::unique_ptr<Mesh>> meshes_;
    };
} // namespace raphEngine
