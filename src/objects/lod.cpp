#include "objects/lod.hpp"

#include <vector>

#include <RaphEngine2/component/camera_component.hpp>
#include <RaphEngine2/objects/mesh_info.hpp>
#include <RaphEngine2/objects/object_mesh.hpp>

namespace raphEngine::objects
{

    const ObjectMesh* Lod::get_lod(size_t level) const
    {
        return lod_meshes_.at(level).get();
    }

    float getManhattanDistance(const glm::vec3& c1, const glm::vec3& c2)
    {
        float dx = std::abs(c2.x - c1.x);
        float dy = std::abs(c2.y - c1.y);
        float dz = std::abs(c2.z - c1.z);
        return dx*dy*dz;
    }

    const ObjectMesh* Lod::get_lod_at(const glm::vec3& object_pos) const
    {
        component::CameraComponent* cam = component::CameraComponent::active_camera;
        const glm::vec3& campera_pos = cam->get_position();
        float far_plane_2 = cam->farPlane * cam->farPlane;

        float distance_m = getManhattanDistance(campera_pos, object_pos);

        float threshold = distance_m / far_plane_2;

        size_t lod_index = 0;

        for (const auto t : transitions_distances_)
        {
            if (t < threshold)
                lod_index++;
            else
                break;
        }

        // Logger::LogDebug("lod ", lod_index, " for a threshold of ", threshold);
        return get_lod(lod_index);
    }

    size_t Lod::get_lod_count() const
    {
        return lod_meshes_.size();
    }

    Lod::Lod(objects::GameObject* parent_object,
             std::initializer_list<MeshInfo> meshes,
             std::shared_ptr<graphics::Shader> shader)
    {
        lod_meshes_.reserve(meshes.size());
        for (const auto& info : meshes)
        {
            lod_meshes_.push_back(std::make_unique<ObjectMesh>(
                parent_object, info, shader.get()));
        }
        calculate_transitions_distances(meshes.size());
    }

    Lod::Lod(objects::GameObject* parent_object, std::vector<MeshInfo>& meshes,
             std::shared_ptr<graphics::Shader> shader)
    {
        lod_meshes_.reserve(meshes.size());
        
        for (const auto& info : meshes)
        {
            lod_meshes_.push_back(std::make_unique<ObjectMesh>(
                parent_object, info, shader.get()));
        }
        calculate_transitions_distances(meshes.size());
    }

    Lod::Lod(objects::GameObject* parent_object, MeshInfo mesh,
             std::shared_ptr<graphics::Shader> shader)
    {
        lod_meshes_.push_back(
            std::make_unique<ObjectMesh>(parent_object, mesh, shader.get()));
            
    }

    void Lod::calculate_transitions_distances(int lod_count, float exponent)
    {
        if (lod_count <= 1)
        {
            return;
        }
        transitions_distances_.reserve(lod_count - 1);

        for (int i = 1; i < lod_count; ++i)
        {
            float t = static_cast<float>(i) / lod_count;
            transitions_distances_.push_back(std::pow(t, exponent));
        }
    }

} // namespace raphEngine::objects
