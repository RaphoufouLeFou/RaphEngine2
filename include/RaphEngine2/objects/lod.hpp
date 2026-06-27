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
        Lod(objects::GameObject* parent_object,
            std::initializer_list<MeshInfo> meshes,
            std::shared_ptr<graphics::Shader> shader, const bool* cast_shadow);
        Lod(objects::GameObject* parent_object, std::vector<MeshInfo>& meshes,
            std::shared_ptr<graphics::Shader> shader, const bool* cast_shadow);
        Lod(objects::GameObject* parent_object, MeshInfo mesh,
            std::shared_ptr<graphics::Shader> shader, const bool* cast_shadow);
        size_t get_lod_count() const;

        const ObjectMesh* get_lod_at(const glm::vec3& object_pos) const;

        Lod(const Lod&) = delete;

    private:
        std::vector<std::unique_ptr<ObjectMesh>> lod_meshes_;
        std::vector<float> transitions_distances_;

        void calculate_transitions_distances(int lod_count,
                                             float exponent = 3.0f);
        const ObjectMesh* get_lod(size_t level) const;
    };
} // namespace raphEngine::objects
