#include "component/mesh_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>
#include "logger/logger.hpp"

#include "imgui.h"

#include "objects/lod.hpp"
#include "objects/mesh_info.hpp"

namespace raphEngine::component
{

    MeshComponent::MeshComponent()
    {
        shader_ = graphics::Shader::loadShader();
        cast_shadows = true;
        outline_ = false;
    }

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
        cast_shadows = true;
        outline_ = false;
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
        outline_ = false;
    }

    void MeshComponent::Start()
    {
        Logger::LogDebug("creating LOD with ", meshes_.size(), " levels");
        lods_ = std::make_unique<objects::Lod>(parent_object, meshes_, shader_,
                                               &cast_shadows, &outline_);
    }

    void MeshComponent::Update()
    {
        render();
    }

    void MeshComponent::ImGuiPrint()
    {
        if (ImGui::TreeNode(get_name().c_str()))
        {
            ImGui::Checkbox("cast shadows", &cast_shadows);
            ImGui::Checkbox("outline", &outline_);

            int val = lods_->get_lod_count();
            ImGui::InputInt("LOD Count", &val);

            ImGui::SeparatorText("Meshes");

            int indexToRemove = -1;
            for (size_t i = 0; i < meshes_.size(); i++)
            {
                ImGui::PushID((int)i);
                if (ImGui::Button("remove"))
                {
                    indexToRemove = (int)i;
                }
                ImGui::SameLine();
                ImGui::Text("%s", meshes_[i].mesh_path.filename().c_str());
                ImGui::PopID();
            }

            if (indexToRemove != -1)
            {
                meshes_.erase(meshes_.begin() + indexToRemove);
                Start();
            }

            ImGui::SeparatorText("Add mesh");
            static char buff[128] = { 0 };
            ImGui::InputText("path", buff, 128);
            ImGui::SameLine();
            if (ImGui::Button("Add"))
            {
                meshes_.push_back(objects::MeshInfo(std::string(buff)));
                Start();
                memset(buff, 0, 128);
            }

            ImGui::TreePop();
        }
    }

    void MeshComponent::render() const
    {
        if (lods_->get_lod_count() > 0)
        {
            lods_->get_lod_at(parent_object->get_transform().get_position())
                ->render();
        }
    }

} // namespace raphEngine::component
