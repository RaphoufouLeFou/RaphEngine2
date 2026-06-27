#include "component/camera_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <vector>

#include "graphics/graphic_api.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <RaphEngine2/logger/logger.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "objects/lod.hpp"
#include "objects/mesh_info.hpp"

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
