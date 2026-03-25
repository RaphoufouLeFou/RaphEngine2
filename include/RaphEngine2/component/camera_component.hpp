#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

#include "component.hpp"
#include "objects/lod.hpp"
#include "objects/mesh_info.hpp"
#include "renderable.hpp"

namespace raphEngine::component
{
    class RAPHENGINE_API CameraComponent : public Component
    {
    public:
        CameraComponent(std::initializer_list<objects::MeshInfo> mesh_lods);
        const std::string component_name = "Render mesh";

        inline const std::string get_name() const override
        {
            return component_name;
        }

        void Start() override;
        void Update() override;

    private:
        float fov;
        float nearPlane;
        float farPlane;
        glm::mat4 view_matrix_;
        glm::mat4 projection_matrix_;
    };
} // namespace raphEngine::component
