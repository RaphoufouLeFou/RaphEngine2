#include "objects/lod.hpp"

#include <vector>

#include "objects/mesh_info.hpp"
#include "objects/object_mesh.hpp"

namespace raphEngine::objects
{
    
    const ObjectMesh* Lod::get_lod(size_t level) const
    {
        return lod_meshes_.at(level).get();
    }

    size_t Lod::get_lod_count() const
    {
        return lod_meshes_.size();
    }

    Lod::Lod(std::initializer_list<MeshInfo> meshes, std::shared_ptr<graphics::Shader> shader)
    {
        lod_meshes_.reserve(meshes.size());
        for (const auto& info : meshes)
        {
            lod_meshes_.push_back(std::make_unique<ObjectMesh>(info, shader.get()));
        }
    }

    Lod::Lod(MeshInfo mesh, std::shared_ptr<graphics::Shader> shader)
    {
        lod_meshes_.push_back(std::make_unique<ObjectMesh>(mesh, shader.get()));
    }

} // namespace raphEngine::objects
