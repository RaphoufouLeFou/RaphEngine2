#include <RaphEngine2/editor/editor_camera.hpp>
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
#include "editor/layout.hpp"

namespace raphEngine
{
    float speed = 10;
    using namespace inputs;
    using namespace objects;

    glm::vec2 lastMousePos = glm::vec2(-1, -1);
    bool lastMouseState = false;

    void EditorCamera::HandleMouseRotation()
    {
        int WindowWidth = graphics::GraphicApi::viewport_res_x;
        int WindowHeight = graphics::GraphicApi::viewport_res_y;
        int WindowPosX = graphics::GraphicApi::viewport_pos_x;
        int WindowPosY = graphics::GraphicApi::viewport_pos_y;

        glm::vec2 mousePos = Mouse::GetMousePos();

        if (Mouse::IsMouseButtonPressed(Mouse::MouseButton::RIGHT)
            && Mouse::IsMouseOnScreen() && Mouse::IsWindowFocused())
        {
            glm::vec2 MiddleScreen = glm::vec2(WindowPosX + WindowWidth / 2,
                                               WindowPosY + WindowHeight / 2);
            if (!lastMouseState)
                mousePos = MiddleScreen;
            glm::vec2 delta = mousePos - MiddleScreen;
            rotation_.x -= delta.y * 0.1f;
            rotation_.z -= delta.x * 0.1f;
            Mouse::SetMouseVisibility(false);
            Mouse::SetMousePosition(MiddleScreen.x, MiddleScreen.y);
            lastMouseState = true;
        }
        else
        {
            Mouse::SetMouseVisibility(true);
            if (lastMouseState)
            {
                glm::vec2 MiddleScreen =
                    glm::vec2(WindowPosX + WindowWidth / 2,
                              WindowPosY + WindowHeight / 2);
                Mouse::SetMousePosition(MiddleScreen.x, MiddleScreen.y);
                // Mouse::SetMousePosition(lastMousePos.x, lastMousePos.y);
            }
            lastMousePos = mousePos;
            lastMouseState = false;
        }
    }

    void EditorCamera::CamUpdate()
    {
        HandleMouseRotation();

        glm::mat4 RotationMat = glm::toMat4(glm::quat(glm::radians(rotation_)));
        glm::vec3 movement = glm::vec3(0);

        float multiplier = 1;

        if (Key::IsKeyPressed(Key::KeyCode::KEY_LEFT_SHIFT))
            multiplier = 15;
        if (Key::IsKeyPressed(Key::KeyCode::KEY_LEFT_CONTROL))
            multiplier = 0.1f;

        if (Key::IsKeyPressed(Key::KeyCode::KEY_W))
            movement.y += speed * multiplier * Time::deltaTime;
        if (Key::IsKeyPressed(Key::KeyCode::KEY_S))
            movement.y -= speed * multiplier * Time::deltaTime;
        if (Key::IsKeyPressed(Key::KeyCode::KEY_D))
            movement.x += speed * multiplier * Time::deltaTime;
        if (Key::IsKeyPressed(Key::KeyCode::KEY_A))
            movement.x -= speed * multiplier * Time::deltaTime;

        glm::vec3 direction = glm::vec3(RotationMat * glm::vec4(movement, 1));

        position_ += direction;

        if (!inputs::Mouse::IsMouseOnScreen())
            return;

        static bool last_pressed = false;

        if (!inputs::Mouse::IsMouseButtonPressed(
                raphEngine::inputs::Mouse::MouseButton::LEFT))
        {
            last_pressed = false;
            return;
        }
        if (last_pressed)
            return;

        last_pressed = true;

        RayInfo OutRayInfo;
        if (RayCast::FromMouseMeshes(&OutRayInfo))
        {
            Logger::LogInfo("Hit on ", OutRayInfo.hitObject->get_name());
            editor::Layout::SelectObject(OutRayInfo.hitObject);
        }
        else
        {
            editor::Layout::UnselectAll();
        }
    }

    const glm::vec3& EditorCamera::get_position() const
    {
        return position_;
    }

    const glm::vec3& EditorCamera::get_rotation() const
    {
        return rotation_;
    }

} // namespace raphEngine
