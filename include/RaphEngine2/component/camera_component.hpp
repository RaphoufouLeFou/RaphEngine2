#pragma once

#include <RaphEngine2/export.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#include "component.hpp"
#include <RaphEngine2/graphics/camera.hpp>

namespace raphEngine::component
{
    class RAPHENGINE_API CameraComponent
        : public Component
        , public Camera
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
        void CamUpdate() override;
#ifdef EDITOR_BUILD
        void ImGuiPrint() override;
#endif
        const glm::vec3& get_position() const override;
        const glm::vec3& get_rotation() const override;

    private:
        REFLECT(CameraComponent, Component, fov_, nearPlane_, farPlane_)
        REFLECT_FACTORY(CameraComponent, Component, "Camera")
    };
} // namespace raphEngine::component
