#include "RaphEngine2/core.hpp"

#include <iostream>

namespace raphEngine
{
    void Core::Init()
    {
        std::cout << "Hello world from RaphEngine2!" << std::endl;
    }

    void Core::Run()
    {
        std::cout << "running now from RaphEngine2!" << std::endl;
    }
}