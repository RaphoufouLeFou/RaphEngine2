#include "camera.hpp"

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

#ifdef EDITOR_BUILD
float speed = 10;

using namespace raphEngine;
using namespace raphEngine::inputs;
using namespace raphEngine::objects;

GameObject* poi = nullptr;

void Camera::Start()
{
    Logger::LogInfo("Starting camera");
    poi = GameObject::find("Ball");
    /*
        auto lods = {
            // MeshInfo("assets/models/pylone3.fbx"),
            MeshInfo("editor/assets/models/untitled.glb"),
        };

        poi->add_component<component::MeshComponent>(lods);
        poi->get_transform().set_scale(glm::vec3(1));
        poi->add_component<component::ColliderComponent>();
        poi->raycast_layer_ = 1;
        */
}

glm::vec2 lastMousePos = glm::vec2(-1, -1);
bool lastMouseState = false;

void Camera::HandleMouseRotation()
{
    int WindowWidth, WindowHeight;
    WindowWidth = graphics::GraphicApi::res_x;
    WindowHeight = graphics::GraphicApi::res_y;

    glm::vec2 mousePos = Mouse::GetMousePos();

    if (Mouse::IsMouseButtonPressed(Mouse::MouseButton::RIGHT)
        && Mouse::IsMouseOnScreen() && Mouse::IsWindowFocused())
    {
        glm::vec2 MiddleScreen = glm::vec2(WindowWidth / 2, WindowHeight / 2);
        if (!lastMouseState)
            mousePos = MiddleScreen;
        glm::vec2 delta = mousePos - MiddleScreen;
        glm::vec3 rot = transform_.get_rotation();
        rot.x -= delta.y * 0.1f;
        rot.z -= delta.x * 0.1f;
        transform_.set_rotation(rot);
        Mouse::SetMouseVisibility(false);
        Mouse::SetMousePosition(MiddleScreen.x, MiddleScreen.y);
        lastMouseState = true;
    }
    else
    {
        Mouse::SetMouseVisibility(true);
        if (lastMouseState)
            Mouse::SetMousePosition(lastMousePos.x, lastMousePos.y);
        lastMousePos = mousePos;
        lastMouseState = false;
    }
}

void Camera::Update()
{
    HandleMouseRotation();

    const glm::vec3& rot = transform_.get_rotation();

    glm::mat4 RotationMat = glm::toMat4(glm::quat(glm::radians(rot)));
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

    transform_.translate(direction);

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
        OutRayInfo.hitObject->ImGui_select();
        // if (poi)
        //     poi->get_transform().set_position(OutRayInfo.hitPoint);
    }
    else
    {
        GameObject::ImGui_unselect();
    }
}
#endif
