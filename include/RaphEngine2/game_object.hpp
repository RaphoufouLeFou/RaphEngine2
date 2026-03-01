#pragma once

#include <string>

#include "renderable.hpp"
#include "transform.hpp"

class GameObject : private Renderable
{
private:
    std::string name_;
    Transform transform_;
};
