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
    }

    void ColliderComponent::add_tri_to_collider_mesh(const objects::Mesh* mesh)
    {
        const glm::mat4 meshModel = mesh->get_model_matrix();

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
            objTri.a =
                glm::vec3(meshModel * glm::vec4(verts[i0].position, 1.0f));
            objTri.b =
                glm::vec3(meshModel * glm::vec4(verts[i1].position, 1.0f));
            objTri.c =
                glm::vec3(meshModel * glm::vec4(verts[i2].position, 1.0f));
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

    void ColliderComponent::update_cached_transform(const glm::mat4& model)
    {
        if (model == cached_model_)
            return;

        cached_model_ = model;
        cached_inv_model_ = glm::inverse(model);
        cached_normal_matrix_ = glm::transpose(glm::mat3(cached_inv_model_));
    }

    void ColliderComponent::build_soa_cache()
    {
#if defined(__AVX2__)
        size_t count = collider_mesh.size();
        size_t paddedCount = ((count + 7) / 8) * 8;

        collider_mesh_soa.ax.assign(paddedCount, 0.0f);
        collider_mesh_soa.ay.assign(paddedCount, 0.0f);
        collider_mesh_soa.az.assign(paddedCount, 0.0f);
        collider_mesh_soa.bx.assign(paddedCount, 0.0f);
        collider_mesh_soa.by.assign(paddedCount, 0.0f);
        collider_mesh_soa.bz.assign(paddedCount, 0.0f);
        collider_mesh_soa.cx.assign(paddedCount, 0.0f);
        collider_mesh_soa.cy.assign(paddedCount, 0.0f);
        collider_mesh_soa.cz.assign(paddedCount, 0.0f);

        for (size_t k = 0; k < count; k++)
        {
            const Utils::Triangle& t = collider_mesh[k];
            collider_mesh_soa.ax[k] = t.a.x;
            collider_mesh_soa.ay[k] = t.a.y;
            collider_mesh_soa.az[k] = t.a.z;
            collider_mesh_soa.bx[k] = t.b.x;
            collider_mesh_soa.by[k] = t.b.y;
            collider_mesh_soa.bz[k] = t.b.z;
            collider_mesh_soa.cx[k] = t.c.x;
            collider_mesh_soa.cy[k] = t.c.y;
            collider_mesh_soa.cz[k] = t.c.z;
        }

        collider_mesh_soa.count = count;
        collider_mesh_soa.paddedCount = paddedCount;

        size_t numBatches = paddedCount / 8;
        collider_mesh_soa.batch_starts.resize(numBatches);
        for (size_t b = 0; b < numBatches; b++)
            collider_mesh_soa.batch_starts[b] = b * 8;
#endif
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
            { bounding_min.x, bounding_min.y, bounding_min.z }, // 0
            { bounding_max.x, bounding_min.y, bounding_min.z }, // 1
            { bounding_max.x, bounding_max.y, bounding_min.z }, // 2
            { bounding_min.x, bounding_max.y, bounding_min.z }, // 3
            { bounding_min.x, bounding_min.y, bounding_max.z }, // 4
            { bounding_max.x, bounding_min.y, bounding_max.z }, // 5
            { bounding_max.x, bounding_max.y, bounding_max.z }, // 6
            { bounding_min.x, bounding_max.y, bounding_max.z } // 7
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