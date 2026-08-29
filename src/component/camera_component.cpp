#include "component/camera_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <RaphEngine2/objects/transform.hpp>
#include <RaphEngine2/objects/game_object.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <RaphEngine2/logger/logger.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "imgui.h"

namespace raphEngine::component
{

    CameraComponent::CameraComponent()
    {
        fov_ = 60;
        nearPlane_ = .01f;
        farPlane_ = 5000.0f;
        Logger::LogDebug("creating camera component");
        if (!get_active_camera())
            set_as_active_camera();
    }

    void CameraComponent::Start()
    {}

    void CameraComponent::Update()
    {}

    void CameraComponent::CamUpdate()
    {}

    void CameraComponent::ImGuiPrint()
    {
        if (ImGui::TreeNode(get_name().c_str()))
        {
            ImGui::DragFloat("FOV", &fov_, 1);
            ImGui::DragFloat("Far plane", &farPlane_, 1);
            ImGui::DragFloat("Near Plane", &nearPlane_, 0.01);
            ImGui::TreePop();
        }
    }

    const glm::vec3& CameraComponent::get_position() const
    {
        return parent_object->get_transform().get_position();
    }

    const glm::vec3& CameraComponent::get_rotation() const
    {
        return parent_object->get_transform().get_rotation();
    }

} // namespace raphEngine::component
