#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>

namespace raphEngine::objects
{
    class RAPHENGINE_API Transform
    {
    public:
        Transform();

        glm::vec3& get_position();
        const glm::vec3& get_position() const;

        glm::vec3& get_rotation();
        const glm::vec3& get_rotation() const;

        glm::vec3& get_scale();
        const glm::vec3& get_scale() const;

        const glm::mat4 get_model_matrix();

        bool can_have_moved = true;

    private:
        void calculate_matrix();

        glm::vec3 position_;
        glm::vec3 rotation_;
        glm::vec3 scale_;

        glm::mat4 model_matrix_;
    };
} // namespace raphEngine::objects
