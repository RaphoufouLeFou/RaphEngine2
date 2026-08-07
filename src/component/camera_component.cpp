#include "component/camera_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <RaphEngine2/objects/transform.hpp>
#include <RaphEngine2/objects/game_object.hpp>
#include "graphics/graphic_api.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <RaphEngine2/logger/logger.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#ifdef EDITOR_BUILD
#    include "imgui.h"
#endif

namespace raphEngine::component
{
    CameraComponent* CameraComponent::active_camera = nullptr;

    void CameraComponent::set_as_active_camera()
    {
        active_camera = this;
    }

    CameraComponent::CameraComponent()
    {
        fov = 60;
        nearPlane = .1f;
        farPlane = 2500.0f;
        Logger::LogDebug("creating camera");
        if (!active_camera)
        {
            set_as_active_camera();
        }
    }

    void CameraComponent::Start()
    {}

    void CameraComponent::Update()
    {}

#ifdef EDITOR_BUILD
    void CameraComponent::ImGuiPrint()
    {
        if (ImGui::TreeNode(get_name().c_str()))
        {
            ImGui::DragFloat("FOV", &fov, 1);
            ImGui::DragFloat("Far plane", &farPlane, 1);
            ImGui::DragFloat("Near Plane", &nearPlane, 0.01);
            ImGui::TreePop();
        }
    }
#endif

    const glm::vec3& CameraComponent::get_position() const
    {
        return parent_object->get_transform().get_position();
    }

    void CameraComponent::calculate_matrices()
    {
        objects::Transform& camera_transform = parent_object->get_transform();

        projection_matrix_ =
            glm::perspective(glm::radians(fov),
                             (float)(graphics::GraphicApi::res_x)
                                 / (float)(graphics::GraphicApi::res_y),
                             nearPlane, farPlane);

        // float coef = 60;
        // float X = (float)(*Renderer::ResX) / 2 / coef;
        // float Y = (float)(*Renderer::ResY) / 2 / coef;
        // ProjectionMatrix = glm::ortho(-X, X, -Y, Y, 0.3f, 300.00f);
        // Camera matrix

        glm::mat4 RotationMat = glm::toMat4(
            glm::quat(glm::radians(camera_transform.get_rotation())));
        glm::vec3 direction = glm::vec3(RotationMat * glm::vec4(0, 1, 0, 1));
        // glm::vec3 right = glm::vec3(RotationMat * glm::vec4(1, 0, 0, 1));
        glm::vec3 up = glm::vec3(RotationMat * glm::vec4(0, 0, 1, 1));
        // glm::vec3 up = glm::cross(right, direction);

        glm::vec3 pos = parent_object->get_transform().get_position();

        view_matrix_ = glm::lookAt(pos, pos + direction, up);
    }

} // namespace raphEngine::component
