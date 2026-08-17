#include "component/collider_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <vector>

#include <algorithm>
#include <glm/glm.hpp>
#include "component/mesh_component.hpp"
#include "graphics/debug.hpp"
#include "objects/lod.hpp"
#include "utils.hpp"
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/graphics/graphic_api.hpp>

#ifdef EDITOR_BUILD
#    include "imgui.h"
#endif

namespace raphEngine::component
{

    ColliderComponent::ColliderComponent()
    {
        started = false;
    }

    ColliderComponent::ColliderComponent(const MeshComponent& mesh_source)
    {
        get_collider_from_mesh_component(mesh_source);
        started = true;
    }

    void ColliderComponent::get_collider_from_mesh_component(
        const MeshComponent& mesh_source)
    {
        const auto object_mesh = mesh_source.lods_->get_lod_at_level(0);
        geometry_ = ColliderGeometryCache::get_or_build(object_mesh);
        Logger::LogDebug(
            "Collider geometry: ", geometry_->triangles.size(),
            " triangles, bounds min=(", geometry_->bounding_min.x, ",",
            geometry_->bounding_min.y, ",", geometry_->bounding_min.z,
            ") max=(", geometry_->bounding_max.x, ",",
            geometry_->bounding_max.y, ",", geometry_->bounding_max.z, ")");
    }

    void ColliderComponent::update_cached_transform(const glm::mat4& model)
    {
        if (model == cached_model_)
            return;

        cached_model_ = model;
        cached_inv_model_ = glm::inverse(model);
        cached_normal_matrix_ = glm::transpose(glm::mat3(cached_inv_model_));
    }

    void ColliderComponent::Start()
    {
        if (!started)
        {
            const MeshComponent* mesh =
                parent_object->get_first_component_of_type<MeshComponent>();
            if (!mesh)
            {
                Logger::LogError("Can't create a collider if there is no mesh "
                                 "on the object");
                return;
            }
            get_collider_from_mesh_component(*mesh);
        }
    }

    void ColliderComponent::Update()
    {
        if (show_bounding_box)
            DebugDrawBoundingBox();
    }

    void ColliderComponent::DebugDrawBoundingBox(const glm::vec3& color)
    {
        // 8 corners of the box, in local space
        const glm::vec3 local_corners[8] = {
            { get_bounding_min().x, get_bounding_min().y,
              get_bounding_min().z },
            { get_bounding_max().x, get_bounding_min().y,
              get_bounding_min().z },
            { get_bounding_max().x, get_bounding_max().y,
              get_bounding_min().z },
            { get_bounding_min().x, get_bounding_max().y,
              get_bounding_min().z },
            { get_bounding_min().x, get_bounding_min().y,
              get_bounding_max().z },
            { get_bounding_max().x, get_bounding_min().y,
              get_bounding_max().z },
            { get_bounding_max().x, get_bounding_max().y,
              get_bounding_max().z },
            { get_bounding_min().x, get_bounding_max().y,
              get_bounding_max().z },
        };

        const glm::mat4 model =
            parent_object->get_transform().get_model_matrix();

        glm::vec3 corners[8];
        for (int i = 0; i < 8; ++i)
        {
            corners[i] = glm::vec3(model * glm::vec4(local_corners[i], 1.0f));
        }

        graphics::Debug* debug = graphics::Debug::getInstance();

        // Bottom face (z = min)
        debug->DrawLine(corners[0], corners[1], color);
        debug->DrawLine(corners[1], corners[2], color);
        debug->DrawLine(corners[2], corners[3], color);
        debug->DrawLine(corners[3], corners[0], color);

        // Top face (z = max)
        debug->DrawLine(corners[4], corners[5], color);
        debug->DrawLine(corners[5], corners[6], color);
        debug->DrawLine(corners[6], corners[7], color);
        debug->DrawLine(corners[7], corners[4], color);

        // Vertical edges connecting bottom to top
        debug->DrawLine(corners[0], corners[4], color);
        debug->DrawLine(corners[1], corners[5], color);
        debug->DrawLine(corners[2], corners[6], color);
        debug->DrawLine(corners[3], corners[7], color);
    }

#ifdef EDITOR_BUILD
    void ColliderComponent::ImGuiPrint()
    {
        if (ImGui::TreeNode(get_name().c_str()))
        {
            ImGui::Checkbox("Show bounding box", &show_bounding_box);
            ImGui::InputFloat3("Bounding max", &bounding_max.x);
            ImGui::InputFloat3("Bounding min", &bounding_min.x);
            ImGui::TreePop();
        }
    }
#endif

} // namespace raphEngine::component