#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>
#include <vector>

#include "RaphEngine2/graphics/shader.hpp"
#include "mesh.hpp"
#include "mesh_info.hpp"

namespace raphEngine::objects
{
    class RAPHENGINE_API ObjectMesh
    {
    public:
        ObjectMesh() = delete;

        static std::shared_ptr<ObjectMesh>
        get_or_create(objects::GameObject* parent_object, const MeshInfo& info,
                      graphics::Shader* shader, const bool* cast_shadow);

        void add_mesh(std::unique_ptr<Mesh> mesh);
        void render() const;
        std::vector<std::unique_ptr<objects::Mesh>> meshes_;

        ObjectMesh(const ObjectMesh&) = delete;
        ObjectMesh& operator=(const ObjectMesh&) = delete;

    private:
        ObjectMesh(objects::GameObject* parent_object, const MeshInfo& info,
                   graphics::Shader* shader, const bool* cast_shadow);

        graphics::Shader* shader_;
        objects::GameObject* parent_object;
        const bool* cast_shadow_;
    };
} // namespace raphEngine::objects