#pragma once

#include <RaphEngine2/export.hpp>
#include <string>

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

        LightComponent();
        const std::string component_name = "Light";

        inline const std::string get_name() const override
        {
            return component_name;
        }

        void Start() override;
        void Update() override;

    };
} // namespace raphEngine::component
