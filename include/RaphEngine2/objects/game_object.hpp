#pragma once

#include <RaphEngine2/export.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "RaphEngine2/component/component.hpp"
#include "RaphEngine2/core.hpp"
#include "transform.hpp"

namespace raphEngine::component
{
    class Component;
}

namespace raphEngine::objects
{

    template <class T>
    concept Comp = requires(T a) {
        { a } -> std::convertible_to<component::Component>;
    };

    class RAPHENGINE_API GameObject
    {
    public:
        GameObject();
        GameObject(const std::string& name);
        GameObject(GameObject& other);
        ~GameObject();

        void greed();

        virtual void Awake()
        {}
        virtual void Start()
        {}
        virtual void Update()
        {}

        std::string& get_name();
        objects::Transform& get_transform();

        std::shared_ptr<GameObject> instanciate();

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

        static GameObject* find(const std::string& name);

        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;
        GameObject& operator=(GameObject&&) = default;

    protected:
        friend raphEngine::Core;

        std::string name_;
        objects::Transform transform_;
        std::vector<std::unique_ptr<component::Component>> components_;

        static std::vector<GameObject*> spawned_game_objects_;
    };
} // namespace raphEngine::objects

#include "game_object.hxx"
