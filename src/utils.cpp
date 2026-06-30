#include "utils.hpp"

namespace raphEngine
{
    glm::vec3 Utils::GetDirectionFromRotation(const glm::vec3& rotation)
    {
        float pitch = rotation.x;
        float yaw = rotation.y;

        glm::vec3 direction;
        direction.x = cos(pitch) * sin(yaw);
        direction.y = -sin(pitch);
        direction.z = -cos(pitch) * cos(yaw);

        return glm::normalize(direction);
    }
} // namespace raphEngine
