#include "component/mesh_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <initializer_list>
#include <memory>
#include <vector>

#include "imgui.h"

#include "objects/lod.hpp"
#include "objects/mesh_info.hpp"

namespace raphEngine::component
{

    MeshComponent::MeshComponent(
        std::initializer_list<objects::MeshInfo> meshes,
        std::shared_ptr<graphics::Shader> shader)
    {
        if (!shader)
        {
            shader = graphics::Shader::loadShader();
        }
        shader_ = shader;
        meshes_ = meshes;
        Logger::LogDebug("initializing mesh with ", meshes.size(), " lods");
    }

    MeshComponent::MeshComponent(objects::MeshInfo mesh,
                                 std::shared_ptr<graphics::Shader> shader)
    {
        if (!shader)
        {
            shader = graphics::Shader::loadShader();
        }
        shader_ = shader;
        meshes_ = { mesh };
        cast_shadows = true;
    }

    void MeshComponent::Start()
    {
        lods_ = std::make_unique<objects::Lod>(parent_object, meshes_, shader_,
                                               &cast_shadows);
    }

    void MeshComponent::Update()
    {
        render();

        ImGui::Begin(parent_object->get_name().c_str());

        ImGui::Text("Transform");
        ImGui::DragFloat3(
            "Position", &this->parent_object->get_transform().get_position().x,
            0.05);
        ImGui::DragFloat3(
            "Rotation", &this->parent_object->get_transform().get_rotation().x,
            0.01);
        ImGui::DragFloat3(
            "Scale", &this->parent_object->get_transform().get_scale().x,
            0.01);

        ImGui::Text("Properties");
        ImGui::Checkbox("cast shadows", &cast_shadows);

        ImGui::End();
    }
    void MeshComponent::render() const
    {
        lods_->get_lod_at(parent_object->get_transform().get_position())
            ->render();

    }

} // namespace raphEngine::component
