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
#include "RaphEngine2/objects/collider_geometry.hpp"

namespace raphEngine::component
{
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

        bool show_bounding_box = false;

        void update_cached_transform(const glm::mat4& model);

        const std::vector<Utils::Triangle>& get_collider_mesh() const
        {
            return geometry_->triangles;
        }
        const TriangleSOA& get_collider_mesh_soa() const
        {
            return geometry_->soa;
        }
        const glm::vec3& get_bounding_min() const
        {
            return geometry_->bounding_min;
        }
        const glm::vec3& get_bounding_max() const
        {
            return geometry_->bounding_max;
        }

        glm::mat4 cached_model_{ 0.0f };
        glm::mat4 cached_inv_model_{ 1.0f };
        glm::mat3 cached_normal_matrix_{ 1.0f };

        TriangleSOA collider_mesh_soa;

    private:
        void build_soa_cache();
        void get_collider_from_mesh_component(const MeshComponent&);
        void add_tri_to_collider_mesh(const objects::Mesh*);
        void calculate_bounding_box();

        std::shared_ptr<const ColliderGeometry> geometry_;

        bool started = false;
    };
} // namespace raphEngine::component
