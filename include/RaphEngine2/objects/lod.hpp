#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "mesh_info.hpp"
#include "object_mesh.hpp"

namespace raphEngine::objects
{
    class ObjectMesh;

    class RAPHENGINE_API Lod
    {
    public:
        Lod(objects::GameObject* parent_object, std::initializer_list<MeshInfo> meshes, std::shared_ptr<graphics::Shader> shader);
        Lod(objects::GameObject* parent_object, std::vector<MeshInfo>& meshes, std::shared_ptr<graphics::Shader> shader);
        Lod(objects::GameObject* parent_object, MeshInfo mesh, std::shared_ptr<graphics::Shader> shader);
        const ObjectMesh* get_lod(size_t level) const;
        size_t get_lod_count() const;

        Lod(const Lod&) = delete;

    private:
        std::vector<std::unique_ptr<ObjectMesh>> lod_meshes_;
    };
} // namespace raphEngine::objects
