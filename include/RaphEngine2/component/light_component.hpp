#pragma once

#include <RaphEngine2/export.hpp>
#include <string>
#include <glm/glm.hpp>

#include "component.hpp"
#include "RaphEngine2/scenes/reflection.hpp"

namespace raphEngine::component
{
    class RAPHENGINE_API LightComponent : public Component
    {
    public:
        enum Type
        {
            DIRECTIONAL,
            SPOT,
            POINT,
        };

        LightComponent(Type light_type = DIRECTIONAL, float intensity = 1.0f,
                       bool cast_shadows = true);
        const std::string component_name = "Light";

        inline const std::string get_name() const override
        {
            return component_name;
        }

        void set_direction(glm::vec3 direction);

        void Start() override;
        void Update() override;

        void ImGuiPrint() override;

        float intensity_;
        bool cast_shadows_ = true;
        Type type;

    private:
        REFLECT(LightComponent, Component, intensity_, cast_shadows_, type)
        REFLECT_FACTORY(LightComponent, Component, "Light")
    };
} // namespace raphEngine::component
