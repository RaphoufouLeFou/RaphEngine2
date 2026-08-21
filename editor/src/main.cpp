#include <iostream>

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/logger/logger.hpp>

int main()
{
    raphEngine::Core::Init();
    raphEngine::objects::GameObject go{};

    go.greed();

    raphEngine::Logger::LogDebug("Engine build");
    raphEngine::Core::Run();
    return 0;
}
