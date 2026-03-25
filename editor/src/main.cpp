#include <iostream>

#include <RaphEngine2/RaphEngine2.hpp>

int main()
{
    raphEngine::Core::Init();
    raphEngine::objects::GameObject go{};
    std::initializer_list<raphEngine::objects::MeshInfo> infos = 
    {
        raphEngine::objects::MeshInfo("assets/models/cube.fbx", raphEngine::objects::Shader::create_shader("", ""), true)
    };
    go.add_component(std::make_unique<raphEngine::component::MeshComponent>(infos));
    go.greed();
    
    std::cout << "Engine build" << std::endl;
    raphEngine::Core::Run();
    return 0;
}
