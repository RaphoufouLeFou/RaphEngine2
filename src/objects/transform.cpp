#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include "objects/transform.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

namespace raphEngine::objects
{
    
        glm::vec3& Transform::get_position()
        {
            can_have_moved = true;
            return position_;
        }

        const glm::vec3& Transform::get_position() const
        {
            return position_;
        }

        glm::vec3& Transform::get_rotation()
        {
            can_have_moved = true;
            return rotation_;
        }

        const glm::vec3& Transform::get_rotation() const
        {
            return rotation_;
        }

        glm::vec3& Transform::get_scale()
        {
            can_have_moved = true;
            return scale_;
        }

        const glm::vec3& Transform::get_scale() const
        {
            return scale_;
        }

        void Transform::calculate_matrix()
        {
            can_have_moved = false;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, position_);
            model = model * glm::toMat4(glm::quat(glm::radians(rotation_)));
            model = glm::scale(model, scale_);
            model_matrix_ = model;
        }
        
        const glm::mat4 Transform::get_model_matrix()
        {
            if (can_have_moved)
            {
                calculate_matrix();
            }
            return model_matrix_;
        }

} // namespace raphEngine::objects
