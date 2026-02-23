#include <iostream>

#ifdef ENGINE_BUILD 

int main()
{
    std::cout << "Engine build" << std::endl;
    return 0;
}

#endif