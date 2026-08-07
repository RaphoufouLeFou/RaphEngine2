#include "component/mesh_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <initializer_list>
#include <memory>
#include <vector>

#ifdef EDITOR_BUILD
#    include "imgui.h"
#endif

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
    }

#ifdef EDITOR_BUILD
    void MeshComponent::ImGuiPrint()
    {
        if (ImGui::TreeNode(get_name().c_str()))
        {
            ImGui::Checkbox("cast shadows", &cast_shadows);
            int val = lods_->get_lod_count();
            ImGui::InputInt("LOD Count", &val);
            ImGui::TreePop();
        }
    }
#endif

    void MeshComponent::render() const
    {
        lods_->get_lod_at(parent_object->get_transform().get_position())
            ->render();
    }

} // namespace raphEngine::component
