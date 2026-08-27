#include "component/light_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <glm/glm.hpp>
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/graphics/graphic_api.hpp>
#include <RaphEngine2/objects/game_object.hpp>

#include "imgui.h"

namespace raphEngine::component
{

    LightComponent::LightComponent(Type light_type, float intensity,
                                   bool cast_shadows)
    {
        if (cast_shadows)
        {
            if (light_type != DIRECTIONAL)
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
        auto& t = parent_object->get_transform();

        glm::vec3 rotation = t.get_rotation();

        t.set_position(direction * 20.0f);

        rotation.x = std::asin(-direction.y);
        rotation.y = std::atan2(direction.x, -direction.z);
        rotation.z = 0.0f;

        t.set_rotation(rotation);
    }

    void LightComponent::Start()
    {}

    void LightComponent::Update()
    {
        graphics::GraphicApi::AddToLightsPool(this);
    }

    void LightComponent::ImGuiPrint()
    {
        if (ImGui::TreeNode(get_name().c_str()))
        {
            ImGui::Checkbox("cast shadows", &cast_shadows_);
            ImGui::DragFloat("intensity", &intensity_, 0.01);
            ImGui::TreePop();
        }
    }

} // namespace raphEngine::component
