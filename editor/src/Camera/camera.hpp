#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/component/camera_component.hpp>
#include <RaphEngine2/component/mesh_component.hpp>

class Camera : public raphEngine::objects::GameObject
{
public:
    Camera()
        : GameObject("Main Camera")
    {
        this->add_component<raphEngine::component::CameraComponent>();
    }

    void Start() override;
    void Update() override;

private:
    void HandleMouseRotation();
};
