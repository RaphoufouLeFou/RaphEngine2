#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>

namespace raphEngine::graphics
{
    struct RAPHENGINE_API FogSettings
    {
        static float density;
        static glm::vec3 fallbackColor;
    };
} // namespace raphEngine::graphics
