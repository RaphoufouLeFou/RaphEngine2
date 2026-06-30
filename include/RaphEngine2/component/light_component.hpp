#pragma once

#include <RaphEngine2/export.hpp>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "component.hpp"

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
                       bool cast_shadows = false);
        const std::string component_name = "Light";

        inline const std::string get_name() const override
        {
            return component_name;
        }

        void set_direction(glm::vec3 direction);

        void Start() override;
        void Update() override;

        float intensity_;
        bool cast_shadows_;
        Type type;
    };
} // namespace raphEngine::component
