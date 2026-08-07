#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float2.hpp>

namespace raphEngine::objects
{
    class GameObject;
}

namespace raphEngine
{
    typedef struct RayInfo
    {
        glm::vec3 hitPoint;
        glm::vec3 hitNormal;
        float hitDistance;
        objects::GameObject* hitObject;
    } RayInfo;

    class RAPHENGINE_API RayCast
    {
    public:
        static bool FromCamera(glm::vec2 screenPos, RayInfo* OutRayInfo,
                               int layer = 0);
        static bool FromMouse(RayInfo* OutRayInfo, int layer = 0);
        static bool FromPoint(glm::vec3 origin, glm::vec3 direction,
                              RayInfo* OutRayInfo, int layer = 0);

        static void DebugUpdate();

    private:
        static bool haveCollision(glm::vec3 origin, glm::vec3 direction,
                                  glm::vec3& out_intersection_point,
                                  glm::vec3& out_normal,
                                  objects::GameObject** objOut, int layer);
    };
} // namespace raphEngine