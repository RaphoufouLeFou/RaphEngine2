#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace raphEngine::objects
{
    class GameObject;

    class RAPHENGINE_API Transform
    {
    public:
        Transform();

        const glm::vec3& get_position() const;
        void set_position(const glm::vec3& p);
        void translate(const glm::vec3& delta);

        const glm::vec3& get_rotation() const;
        void set_rotation(const glm::vec3& p);
        void rotate(const glm::vec3& delta);

        const glm::vec3& get_scale() const;
        void set_scale(const glm::vec3& p);
        void scale_by(const glm::vec3& delta);

        const glm::mat4 get_model_matrix();

        bool can_have_moved = true;
        GameObject* parent_object;

        static std::vector<Transform*> root_childs;

        void set_parent(Transform* parent);
        Transform* get_parent() const;

        void add_child(Transform* child);
        const std::vector<Transform*>& get_children();
        Transform* get_child(size_t index) const;

    private:
        void calculate_matrix();
        friend GameObject;

        glm::vec3 position_;
        glm::vec3 rotation_;
        glm::vec3 scale_;

        Transform* parent_ = nullptr;
        std::vector<Transform*> children_;

        glm::mat4 model_matrix_;
    };
} // namespace raphEngine::objects
