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
#ifdef EDITOR_BUILD
        void ImGuiPrint() override;
#endif
        std::vector<Utils::Triangle> collider_mesh;
        glm::vec3 bounding_max;
        glm::vec3 bounding_min;

    private:
        void get_collider_from_mesh_component(const MeshComponent&);
        void add_tri_to_collider_mesh(const objects::Mesh*);
        void calculate_bounding_box();
    };
} // namespace raphEngine::component
