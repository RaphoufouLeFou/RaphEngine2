#include "component/light_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include "objects/lod.hpp"
#include "objects/mesh_info.hpp"
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/graphics/graphic_api.hpp>

namespace raphEngine::component
{

    LightComponent::LightComponent(Type light_type, float intensity,
                                   bool cast_shadows)
    {
        if (cast_shadows)
        {
            if (type != DIRECTIONAL)
            {
                Logger::LogError("A light other than a directional light ",
                                 "cannot cast shaodws for now!");
                cast_shadows = false;
            }
        }

        cast_shadows_ = cast_shadows;
        intensity_ = intensity;
        type = light_type;
    }

    void LightComponent::set_direction(glm::vec3 direction)
    {
        direction = glm::normalize(direction);
        glm::vec3& rotation = parent_object->get_transform().get_rotation();

        rotation.x = std::asin(-direction.y);
        rotation.y = std::atan2(direction.x, -direction.z);
        rotation.z = 0.0f;
    }

    void LightComponent::Start()
    {}

    void LightComponent::Update()
    {
        graphics::GraphicApi::AddToLightsPool(this);
    }

} // namespace raphEngine::component
