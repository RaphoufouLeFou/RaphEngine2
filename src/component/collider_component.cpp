#include "component/collider_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <vector>

#include <algorithm>
#include <glm/glm.hpp>
#include "component/mesh_component.hpp"
#include "objects/lod.hpp"
#include "objects/mesh_info.hpp"
#include "utils.hpp"
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/graphics/graphic_api.hpp>

#ifdef EDITOR_BUILD
#    include "imgui.h"
#    include "misc/cpp/imgui_stdlib.h"
#endif

namespace raphEngine::component
{

    ColliderComponent::ColliderComponent()
    {
        const MeshComponent* mesh =
            parent_object->get_first_component_of_type<MeshComponent>();
        if (!mesh)
        {
            Logger::LogError(
                "Can't create a collider if there is no mesh on the object");
            return;
        }
        get_collider_from_mesh_component(*mesh);
    }

    ColliderComponent::ColliderComponent(const MeshComponent& mesh_source)
    {
        get_collider_from_mesh_component(mesh_source);
    }

    void ColliderComponent::get_collider_from_mesh_component(
        const MeshComponent& mesh_source)
    {
        const auto object_mesh = mesh_source.lods_->get_lod_at_level(0);
        size_t mesh_count = object_mesh->meshes_.size();
        for (size_t i = 0; i < mesh_count; i++)
        {
            add_tri_to_collider_mesh(object_mesh->meshes_[i].get());
        }
        calculate_bounding_box();
    }

    void ColliderComponent::add_tri_to_collider_mesh(const objects::Mesh* mesh)
    {
        glm::mat4 Model = mesh->get_model_matrix();
        const auto& verts = mesh->get_vertices();
        const auto& indices = mesh->get_indices();
        bool isIndexed = !indices.empty();
        size_t triSource = isIndexed ? indices.size() : verts.size();
        int triCount = static_cast<int>(triSource / 3);

        collider_mesh.reserve(collider_mesh.size() + triCount);

        for (int k = 0; k < triCount; k++)
        {
            unsigned int i0, i1, i2;
            if (isIndexed)
            {
                i0 = indices[k * 3];
                i1 = indices[k * 3 + 1];
                i2 = indices[k * 3 + 2];
            }
            else
            {
                i0 = static_cast<unsigned int>(k * 3);
                i1 = static_cast<unsigned int>(k * 3 + 1);
                i2 = static_cast<unsigned int>(k * 3 + 2);
            }

            Utils::Triangle objTri;

            objTri.a = Model * glm::vec4(verts[i0].position, 1);
            objTri.b = Model * glm::vec4(verts[i1].position, 1);
            objTri.c = Model * glm::vec4(verts[i2].position, 1);

            collider_mesh.push_back(objTri);
        }
    }

    void ColliderComponent::calculate_bounding_box()
    {
        if (collider_mesh.size() == 0)
        {
            bounding_min = glm::vec3{ 0 };
            bounding_max = glm::vec3{ 0 };
            return;
        }

        bounding_min = collider_mesh.at(0).a;
        bounding_max = bounding_min;

        for (const Utils::Triangle& t : collider_mesh)
        {
            for (const auto& p : { t.a, t.b, t.c })
            {
                bounding_min.x = std::min(bounding_min.x, p.x);
                bounding_min.y = std::min(bounding_min.y, p.y);
                bounding_min.z = std::min(bounding_min.z, p.z);

                bounding_max.x = std::max(bounding_max.x, p.x);
                bounding_max.y = std::max(bounding_max.y, p.y);
                bounding_max.z = std::max(bounding_max.z, p.z);
            }
        }
    }

    void ColliderComponent::Start()
    {}

    void ColliderComponent::Update()
    {}

#ifdef EDITOR_BUILD
    void ColliderComponent::ImGuiPrint()
    {
        if (ImGui::TreeNode(get_name().c_str()))
        {
            ImGui::Text("Collider infos");
            ImGui::TreePop();
        }
    }
#endif

} // namespace raphEngine::component
