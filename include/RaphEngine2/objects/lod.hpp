#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

#include <RaphEngine2/export.hpp>
#include "object_mesh.hpp"
#include "mesh_info.hpp"

namespace raphEngine::objects
{
    class RAPHENGINE_API Lod
    {
    public:
        Lod(std::initializer_list<MeshInfo> meshes);
        const ObjectMesh* get_lod(size_t level) const;
        size_t get_lod_count() const;

        Lod& operator=(std::initializer_list<MeshInfo> meshes);

    private:
        std::vector<std::unique_ptr<ObjectMesh>> lod_meshes_;
    };
}
