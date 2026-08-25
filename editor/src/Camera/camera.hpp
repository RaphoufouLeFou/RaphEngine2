#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/component/camera_component.hpp>
#include <RaphEngine2/component/mesh_component.hpp>
#include "scenes/reflection.hpp"

#ifdef ENGINE_BUILD
using namespace raphEngine::objects;

class Camera : public GameObject
{
public:
    Camera()
        : GameObject("Main Camera")
    {
        Logger::LogDebug("Spawning !!!");
    }

    void Start() override;
    void Update() override;

    int id;

private:
    void HandleMouseRotation();

    REFLECT(Camera, GameObject, id)
    REFLECT_FACTORY(Camera, GameObject, "Camera")
};
#endif
