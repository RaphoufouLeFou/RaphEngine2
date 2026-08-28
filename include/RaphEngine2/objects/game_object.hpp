#pragma once

#include <RaphEngine2/export.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "RaphEngine2/raycast/raycast.hpp"
#include "RaphEngine2/component/component.hpp"
#include "RaphEngine2/core.hpp"
#include "RaphEngine2/scenes/reflection.hpp"
#include "transform.hpp"

namespace raphEngine::component
{
    class Component;
}

namespace raphEngine::emgine
{
    class Layout;
}

namespace raphEngine::objects
{
    class Transform;

    template <class T>
    concept Comp = requires(T* a) {
        { a } -> std::convertible_to<component::Component*>;
    };

    class RAPHENGINE_API GameObject
    {
        friend raphEngine::RayCast;

    public:
        GameObject();
        GameObject(const std::string& name);
        GameObject(const GameObject& other);
        ~GameObject();

        void greed();

        virtual void Awake()
        {}
        virtual void Start() {};
        virtual void Update() {};

        std::string& get_name();
        objects::Transform& get_transform();

        template <Comp T, class... Args>
        T* add_component(Args&&... args);

        component::Component* get_component(size_t index);
        component::Component* get_component(const std::string& name);

        template <Comp T>
        std::vector<T*> get_all_component_of_type();

        template <Comp T>
        T* get_first_component_of_type();

        void remove_component(size_t index);
        void remove_component(const std::string& name);

        template <Comp T>
        void remove_all_component_of_type();

        template <Comp T>
        void remove_first_component_of_type();

        void start_components();
        void update_components();

        nlohmann::json toJson() const;
        void fromJson(const nlohmann::json& j);

        static GameObject* instanciate(const GameObject&);
        static GameObject* find(const std::string& name);
        static void destroy(GameObject&);

        GameObject& operator=(const GameObject&) = delete;
        GameObject& operator=(GameObject&&) = delete;

        int raycast_layer_ = 0;

    protected:
        friend raphEngine::Core;

        bool is_active = true;
        std::string name_;
        objects::Transform transform_;
        std::vector<std::unique_ptr<component::Component>> components_;

    private:
        bool has_started = false;
        unsigned int id_ = 0;
        std::string uuid_;
        void pre_update();
        void destroy_internal();

        friend class Layout;
        friend Transform;
        void ImGui_layout();
        void ImGui_update();

        void add_component(std::unique_ptr<component::Component> c);

        REFLECT_ROOT(GameObject, name_, id_, uuid_, is_active, raycast_layer_)
        REFLECT_FACTORY(GameObject, GameObject, "GameObject")
    };
} // namespace raphEngine::objects

#include "game_object.hxx"
