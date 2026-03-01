#include <iostream>

#include <RaphEngine2/RaphEngine2.hpp>

int main()
{
    raphEngine::Core::Init();
    raphEngine::GameObject go{};
    go.greed();
    
    std::cout << "Engine build" << std::endl;
    return 0;
}
