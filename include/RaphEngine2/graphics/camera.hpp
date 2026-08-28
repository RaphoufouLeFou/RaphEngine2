#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/component/mesh_component.hpp>

namespace raphEngine
{
    class Camera
    {
    public:
        Camera()
        {}

        void calculate_matrices();
        virtual const glm::vec3& get_position() const = 0;
        virtual const glm::vec3& get_rotation() const = 0;

        virtual void CamUpdate() = 0;

        float get_fov() const;
        float get_nearPlane() const;
        float get_farPlane() const;
        glm::mat4 get_view_matrix_() const;
        glm::mat4 get_projection_matrix_() const;
        static Camera* get_active_camera();
        void set_active_camera(Camera*);
        void set_as_active_camera();

    protected:
        float fov_ = 60;
        float nearPlane_ = .4f;
        float farPlane_ = 2000.0f;
        glm::mat4 view_matrix_;
        glm::mat4 projection_matrix_;

    private:
        static Camera* active_camera;
    };
} // namespace raphEngine
