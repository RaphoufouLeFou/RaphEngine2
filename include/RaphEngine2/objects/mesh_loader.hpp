#pragma once

#include <glm/glm.hpp>

#include <RaphEngine2/export.hpp>
#include "mesh_info.hpp"
#include <memory>

#include "object_mesh.hpp"

namespace raphEngine::objects
{
    class ObjectMesh;
    class RAPHENGINE_API MeshLoader
    {
        public:
        static std::unique_ptr<ObjectMesh> loadMesh(const MeshInfo& mesh_info);
    };
}
