#pragma once

#include <glm/glm.hpp>

#include <RaphEngine2/export.hpp>
#include "mesh_info.hpp"
#include <memory>

#include "object_mesh.hpp"

namespace raphEngine::objects
{
    class RAPHENGINE_API MeshLoader
    {
        public:
        class ObjectMesh;
        static std::unique_ptr<ObjectMesh> loadMesh(const MeshInfo& mesh_info);

    };
}
