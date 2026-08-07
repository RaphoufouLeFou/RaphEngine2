#include "objects/transform.hpp"
#include <cstddef>

#define GLM_ENABLE_EXPERIMENTAL
#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

namespace raphEngine::objects
{

    std::vector<Transform*> Transform::root_childs;
    Transform::Transform()
    {
        position_ = glm::vec3(0);
        rotation_ = glm::vec3(0);
        scale_ = glm::vec3(1);
        root_childs.push_back(this);
    }

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

    void Transform::set_parent(Transform* parent)
    {
        std::vector<Transform*>& parent_list =
            parent_ ? parent_->children_ : root_childs;

        auto position = std::find(parent_list.begin(), parent_list.end(), this);
        if (position != parent_list.end())
            parent_list.erase(position);

        parent_ = parent;
        if (!parent_)
        {
            root_childs.push_back(this);
        }
        else
        {
            parent_->children_.push_back(this);
        }
    }

    Transform* Transform::get_parent() const
    {
        return parent_;
    }

    void Transform::add_child(Transform* child)
    {
        children_.push_back(child);
        child->set_parent(this);
    }

    const std::vector<Transform*>& Transform::get_children()
    {
        return children_;
    }

    Transform* Transform::get_child(size_t index) const
    {
        return children_.at(index);
    }

    const glm::mat4 Transform::get_model_matrix()
    {
        if (can_have_moved)
        {
            calculate_matrix();
        }
        if (!parent_)
            return model_matrix_;
        else
            return parent_->get_model_matrix() * model_matrix_;
    }

} // namespace raphEngine::objects
