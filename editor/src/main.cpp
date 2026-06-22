#include <iostream>

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/logger/logger.hpp>

int main()
{
    raphEngine::Core::Init();
    raphEngine::objects::GameObject go{};

    /*std::initializer_list<raphEngine::objects::MeshInfo> infos = 
    {
        // raphEngine::objects::MeshInfo("assets/models/cube.fbx", raphEngine::objects::Shader::create_shader("", ""), true)
    };
    go.add_component(std::make_unique<raphEngine::component::MeshComponent>(infos, nullptr));
    */

    go.greed();
    
    raphEngine::Logger::LogDebug("Engine build");
    raphEngine::Core::Run();
    return 0;
}
