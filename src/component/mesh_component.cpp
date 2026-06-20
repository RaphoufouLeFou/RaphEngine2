
#include "component/mesh_component.hpp"

#include <RaphEngine2/graphics/shader.hpp>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <vector>

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
        lods_ = std::make_unique<objects::Lod>(meshes, shader);
        Start();
    }

    MeshComponent::MeshComponent(objects::MeshInfo mesh, std::shared_ptr<graphics::Shader> shader)
    {
        if (!shader)
        {
            shader = graphics::Shader::loadShader();
        }
        shader_ = shader;
        lods_ = std::make_unique<objects::Lod>(mesh, shader);
        Start();
    }

    void MeshComponent::Start()
    {}
    void MeshComponent::Update()
    {
        std::cout << "updating me !!\n";
        render();
    }
    void MeshComponent::render()
    {
        lods_->get_lod(0)->render();
    }

} // namespace raphEngine::component
