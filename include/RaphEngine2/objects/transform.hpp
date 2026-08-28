#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <RaphEngine2/utils.hpp>

namespace raphEngine::objects
{
    class GameObject;

    class RAPHENGINE_API Transform
    {
    public:
        Transform();
        ~Transform();

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
        GameObject* parent_object = nullptr;

        static std::vector<Transform*> root_childs;

        void set_parent(Transform* parent, bool worldPositionStay = true);
        Transform* get_parent() const;

        void add_child(Transform* child);
        const std::vector<Transform*>& get_children();
        Transform* get_child(size_t index) const;

        friend void to_json(nlohmann::json& j, const Transform& t)
        {
            j = { { "position", t.position_ },
                  { "rotation", t.rotation_ },
                  { "scale", t.scale_ } };
        }
        friend void from_json(const nlohmann::json& j, Transform& t)
        {
            t.position_ = j.at("position").get<glm::vec3>();
            t.rotation_ = j.at("rotation").get<glm::vec3>();
            t.scale_ = j.at("scale").get<glm::vec3>();
        }

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
