#pragma once

#include <RaphEngine2/export.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#include "component.hpp"
#include "objects/mesh_info.hpp"

namespace raphEngine::component
{
    class RAPHENGINE_API CameraComponent : public Component
    {
    public:
        CameraComponent();
        const std::string component_name = "Camera";

        inline const std::string get_name() const override
        {
            return component_name;
        }

        void Start() override;
        void Update() override;

        void set_as_active_camera();
        void calculate_matrices();

        static CameraComponent* active_camera;

        float fov;
        float nearPlane;
        float farPlane;
        glm::mat4 view_matrix_;
        glm::mat4 projection_matrix_;
    };
} // namespace raphEngine::component
