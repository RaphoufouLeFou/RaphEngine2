#pragma once

#include "export.hpp"

class RAPHENGINE_API Renderable
{
    virtual void render() const = 0;
};
