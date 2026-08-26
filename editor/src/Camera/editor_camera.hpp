#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/component/camera_component.hpp>
#include <RaphEngine2/component/mesh_component.hpp>
#include "graphics/camera.hpp"
#include "logger/logger.hpp"

#ifdef EDITOR_BUILD
namespace raphEngine
{
    class EditorCamera : public Camera
    {
    public:
        EditorCamera()
        {
            raphEngine::Logger::LogDebug("Spawning editor cam !!!");
            set_as_active_camera();
        }

        void CamUpdate() override;
        const glm::vec3& get_position() const override;
        const glm::vec3& get_rotation() const override;

    private:
        void HandleMouseRotation();

        glm::vec3 rotation_;
        glm::vec3 position_ = glm::vec3{ 0, -5, 2 };
    };
} // namespace raphEngine

#endif
