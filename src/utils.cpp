#include "utils.hpp"

namespace raphEngine
{
    glm::vec3 Utils::GetDirectionFromRotation(const glm::vec3& rotation)
    {
        float pitch = glm::radians(rotation.x);
        float yaw = glm::radians(rotation.y);

        glm::vec3 direction;
        direction.x = cos(pitch) * sin(yaw);
        direction.y = -sin(pitch);
        direction.z = -cos(pitch) * cos(yaw);

        return glm::normalize(direction);
    }

    glm::vec3 Utils::GetForwardFromModelMatrix(const glm::mat4& model)
    {
        return glm::normalize(glm::vec3(model[1]));
    }
} // namespace raphEngine
