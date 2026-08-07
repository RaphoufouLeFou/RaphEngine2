#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/ext/vector_float3.hpp>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "component.hpp"
#include "RaphEngine2/component/mesh_component.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "RaphEngine2/utils.hpp"

namespace raphEngine::component
{
    struct TriangleSOA
    {
        std::vector<float> ax, ay, az, bx, by, bz, cx, cy, cz;
        std::vector<size_t> batch_starts; // cached once: 0, 8, 16, ...
        size_t count = 0;
        size_t paddedCount = 0;
    };

    class RAPHENGINE_API ColliderComponent : public Component
    {
    public:
        ColliderComponent();
        ColliderComponent(const MeshComponent& mesh_source);
        const std::string component_name = "Collider";

        inline const std::string get_name() const override
        {
            return component_name;
        }

        void Start() override;
        void Update() override;
        void DebugDrawBoundingBox(const glm::vec3& color = glm::vec3{ 0, 0,
                                                                      1 });
#ifdef EDITOR_BUILD
        void ImGuiPrint() override;
#endif
        std::vector<Utils::Triangle> collider_mesh;
        glm::vec3 bounding_min{ 0.0f };
        glm::vec3 bounding_max{ 0.0f };

        void update_cached_transform(const glm::mat4& model);

        glm::mat4 cached_model_{ 0.0f }; // != any real model matrix so the
        glm::mat4 cached_inv_model_{ 1.0f }; // first raycast always recomputes
        glm::mat3 cached_normal_matrix_{ 1.0f };

        TriangleSOA collider_mesh_soa;

    private:
        void build_soa_cache();
        void get_collider_from_mesh_component(const MeshComponent&);
        void add_tri_to_collider_mesh(const objects::Mesh*);
        void calculate_bounding_box();
    };
} // namespace raphEngine::component
