#pragma once

#include <RaphEngine2/export.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "component/component.hpp"
#include "core.hpp"
#include "transform.hpp"

namespace raphEngine::objects
{
    class RAPHENGINE_API GameObject
    {
    public:
        GameObject(const std::string& name = "New GameObject");
        GameObject(GameObject& other);
        ~GameObject() = default;

        void greed();

        virtual void Awake()
        {}
        virtual void Start()
        {}
        virtual void Update()
        {}

        std::string& get_name();

        std::shared_ptr<GameObject> instanciate();

        void add_component(std::unique_ptr<component::Component> component);
        component::Component* get_component(size_t index);
        component::Component* get_component(const std::string& name);

        template <class T>
        std::vector<T*> get_all_component_of_type();

        template <class T>
        T* get_first_component_of_type();

        void remove_component(size_t index);
        void remove_component(const std::string& name);

        template <class T>
        void remove_all_component_of_type();

        template <class T>
        void remove_first_component_of_type();

        void start_components();
        void update_components();

        static GameObject* find(const std::string& name);

        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;
        GameObject& operator=(GameObject&&) = default;

    private:
        friend raphEngine::Core;

        std::string name_;
        objects::Transform transform_;
        std::vector<std::unique_ptr<component::Component>> components_;

        static std::vector<GameObject*> spawned_game_objects_;
    };
} // namespace raphEngine::objects

#include "game_object.hxx"
