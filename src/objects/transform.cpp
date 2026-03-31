#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include "objects/transform.hpp"

namespace raphEngine::objects
{
    
        glm::vec3& Transform::get_position()
        {
            return position_;
        }
        glm::vec3& Transform::get_rotation()
        {
            return rotation_;
        }
        glm::vec3& Transform::get_scale()
        {
            return scale_;
        }
        void Transform::calculate_matrix()
        {
            // TODO
        }
} // namespace raphEngine::objects
