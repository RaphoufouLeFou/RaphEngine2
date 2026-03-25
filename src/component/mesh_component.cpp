
#include "component/mesh_component.hpp"
#include <memory>
#include "objects/lod.hpp"
#include <iostream>

namespace raphEngine::component
{
    MeshComponent::MeshComponent(std::initializer_list<objects::MeshInfo> meshes)
    {
        lods_ = std::make_unique<objects::Lod>(meshes);
        Start();
    }

    void MeshComponent::Start()
    {
        
    }
    void MeshComponent::Update()
    {
        std::cout << "updateing me !!\n";
    }
    void MeshComponent::render()
    {
        
    }


} // namespace raphEngine::settings
