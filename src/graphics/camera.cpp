#include <RaphEngine2/graphics/camera.hpp>

#include <RaphEngine2/component/collider_component.hpp>
#include <RaphEngine2/component/mesh_component.hpp>
#include <RaphEngine2/graphics/graphic_api.hpp>
#include <RaphEngine2/inputs/keyboard.hpp>
#include <RaphEngine2/inputs/mouse.hpp>
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/objects/game_object.hpp>
#include <RaphEngine2/raycast/raycast.hpp>
#include <RaphEngine2/time_utils.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace raphEngine
{

    Camera* Camera::active_camera = nullptr;

    void Camera::calculate_matrices()
    {
        projection_matrix_ = glm::perspective(
            glm::radians(fov_),
            static_cast<float>(graphics::GraphicApi::res_x)
                / static_cast<float>(graphics::GraphicApi::res_y),
            nearPlane_, farPlane_);

        glm::mat4 RotationMat =
            glm::toMat4(glm::quat(glm::radians(get_rotation())));
        glm::vec3 direction = glm::vec3(RotationMat * glm::vec4(0, 1, 0, 1));
        glm::vec3 up = glm::vec3(RotationMat * glm::vec4(0, 0, 1, 1));

        const glm::vec3& pos = get_position();

        view_matrix_ = glm::lookAt(pos, pos + direction, up);
    }

    void Camera::set_as_active_camera()
    {
        active_camera = this;
    }

    float Camera::get_fov() const
    {
        return fov_;
    }

    float Camera::get_nearPlane() const
    {
        return nearPlane_;
    }

    float Camera::get_farPlane() const
    {
        return farPlane_;
    }

    glm::mat4 Camera::get_view_matrix_() const
    {
        return view_matrix_;
    }

    glm::mat4 Camera::get_projection_matrix_() const
    {
        return projection_matrix_;
    }

    Camera* Camera::get_active_camera()
    {
        return active_camera;
    }

    void Camera::set_active_camera(Camera* cam)
    {
        active_camera = cam;
    }

} // namespace raphEngine
