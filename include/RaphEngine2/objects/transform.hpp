#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>

namespace raphEngine::objects
{
    class RAPHENGINE_API Transform
    {
    public:
        glm::vec3& get_position();
        glm::vec3& get_rotation();
        glm::vec3& get_scale();

    private:
        void calculate_matrix();

        glm::vec3 position_;
        glm::vec3 rotation_;
        glm::vec3 scale_;

        glm::mat4 model_matrix_;
    };
} // namespace raphEngine::objects
