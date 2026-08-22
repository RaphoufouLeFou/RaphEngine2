#include "objects/transform.hpp"
#include <atomic>
#include <cmath>
#include <cstddef>
#include <string>
#include "logger/logger.hpp"
#include "time_utils.hpp"

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

    Transform::~Transform()
    {
        set_parent(nullptr);
        auto position = std::find(root_childs.begin(), root_childs.end(), this);
        if (position != root_childs.end())
            root_childs.erase(position);
    }

    const glm::vec3& Transform::get_position() const
    {
        return position_;
    }

    void Transform::set_position(const glm::vec3& p)
    {
        position_ = p;
        can_have_moved = true;
    }

    void Transform::translate(const glm::vec3& delta)
    {
        position_ += delta;
        can_have_moved = true;
    }

    const glm::vec3& Transform::get_rotation() const
    {
        return rotation_;
    }

    void Transform::set_rotation(const glm::vec3& p)
    {
        rotation_ = p;
        can_have_moved = true;
    }

    void Transform::rotate(const glm::vec3& delta)
    {
        rotation_ += delta;
        can_have_moved = true;
    }

    const glm::vec3& Transform::get_scale() const
    {
        return scale_;
    }

    void Transform::set_scale(const glm::vec3& p)
    {
        scale_ = p;
        can_have_moved = true;
    }

    void Transform::scale_by(const glm::vec3& delta)
    {
        scale_ += delta;
        can_have_moved = true;
    }

    void Transform::calculate_matrix()
    {
        can_have_moved = false;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position_);
        model = model * glm::toMat4(glm::quat(glm::radians(rotation_)));
        model = glm::scale(model, scale_);
        if (parent_)
            model_matrix_ = parent_->get_model_matrix() * model;
        else
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
        return model_matrix_;
    }

} // namespace raphEngine::objects
