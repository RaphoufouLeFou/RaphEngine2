#pragma once

#include <nlohmann/json.hpp>
#define GLM_ENABLE_EXPERIMENTAL

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
        static glm::vec3 GetForwardFromModelMatrix(const glm::mat4& model);
    };
} // namespace raphEngine

namespace glm
{
    inline void to_json(nlohmann::json& j, const vec3& v)
    {
        j = { { "x", v.x }, { "y", v.y }, { "z", v.z } };
    }
    inline void from_json(const nlohmann::json& j, vec3& v)
    {
        v.x = j.at("x").get<float>();
        v.y = j.at("y").get<float>();
        v.z = j.at("z").get<float>();
    }
} // namespace glm
