#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <string>

#include "export.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace raphEngine
{
    class RAPHENGINE_API Utils
    {
    public:
        struct Triangle
        {
            glm::vec3 a;
            glm::vec3 b;
            glm::vec3 c;
        };

        static glm::vec3 GetDirectionFromRotation(const glm::vec3& rotation);
    };
} // namespace raphEngine
