#include <execution>
#include <objects/game_object.hpp>
#include <string>
#include "RaphEngine2/raycast/raycast.hpp"
#include "component/camera_component.hpp"
#include "component/collider_component.hpp"
#include "component/mesh_component.hpp"
#include "graphics/debug.hpp"
#include "graphics/graphic_api.hpp"
#include "inputs/mouse.hpp"
#include "logger/logger.hpp"
#include "time_utils.hpp"
#include "utils.hpp"

namespace raphEngine
{

    glm::vec3 GetDirectionFromScreen(glm::vec2 screenPos)
    {
        float ndcX = (2.0f * screenPos.x) / graphics::GraphicApi::res_x - 1.0f;
        float ndcY =
            1.0f - (2.0f * screenPos.y) / graphics::GraphicApi::res_y; // Flip Y

        // Clip Space Coordinates
        glm::vec4 clipCoords(ndcX, ndcY, -1.0f, 1.0f);

        // Eye Space (View Space)

        glm::vec4 eyeCoords =
            inverse(
                component::CameraComponent::active_camera->projection_matrix_)
            * clipCoords;
        eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0,
                              0.0); // w = 0 for direction

        // World Space
        glm::vec4 worldCoords =
            inverse(component::CameraComponent::active_camera->view_matrix_)
            * eyeCoords;
        glm::vec3 rayDirection = normalize(glm::vec3(worldCoords));
        return rayDirection;
    }

    // Moller-Trumbore ray/triangle intersection (adapted from Wikipedia)
    bool ray_intersects_triangle(const glm::vec3& ray_origin,
                                 const glm::vec3& ray_vector,
                                 const Utils::Triangle& tri,
                                 glm::vec3& out_intersection_point)
    {
        constexpr float epsilon = std::numeric_limits<float>::epsilon();

        glm::vec3 edge1 = tri.b - tri.a;
        glm::vec3 edge2 = tri.c - tri.a;
        glm::vec3 ray_cross_e2 = cross(ray_vector, edge2);
        float det = dot(edge1, ray_cross_e2);

        if (det > -epsilon && det < epsilon)
            return false; // This ray is parallel to this triangle.

        float inv_det = 1.0f / det;
        glm::vec3 s = ray_origin - tri.a;
        float u = inv_det * dot(s, ray_cross_e2);

        if (u < -epsilon || u > 1.0f + epsilon)
            return false;

        glm::vec3 s_cross_e1 = cross(s, edge1);
        float v = inv_det * dot(ray_vector, s_cross_e1);

        if (v < -epsilon || u + v > 1.0f + epsilon)
            return false;

        // At this stage we can compute t to find out where the intersection
        // point is on the line.
        float t = inv_det * dot(edge2, s_cross_e1);

        if (t > epsilon) // ray intersection
        {
            out_intersection_point = glm::vec3(ray_origin + ray_vector * t);
            return true;
        }
        else // This means that there is a line intersection but not a ray
             // intersection.
            return false;
    }

    bool InInfluenceSphere(glm::vec3 rayOrigin, glm::vec3 rayDirection,
                           glm::vec3 center, float radius)
    {
        glm::vec3 oc = center - rayOrigin;
        float a = dot(rayDirection, rayDirection);
        float b = 2.0f * dot(rayDirection, oc);
        float c = dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        return (discriminant > 0);
    }

    glm::vec3 GetNewDirection(glm::vec3 origin, glm::vec3 LocalOrigin,
                              glm::vec3 direction, glm::mat4 InvModel)
    {
        glm::vec3 DirectionPoint = origin + direction;
        glm::vec3 TransformedDirPoint = InvModel * glm::vec4(DirectionPoint, 1);
        glm::vec3 NewDirection = TransformedDirPoint - LocalOrigin;
        return glm::normalize(NewDirection);
    }

    void RayCast::DebugUpdate()
    {}

    bool RayCast::haveCollision(glm::vec3 origin, glm::vec3 direction,
                                glm::vec3& out_intersection_point,
                                glm::vec3& out_normal,
                                objects::GameObject** objOut, int layer)
    {
        glm::vec3 oldIntersectionPoint = glm::vec3(0);
        bool hitFound = false;

        int objCount =
            static_cast<int>(objects::GameObject::spawned_game_objects_.size());

        for (int i = 0; i < objCount; i++)
        {
            objects::GameObject* obj =
                objects::GameObject::spawned_game_objects_[i];
            if (obj->raycast_layer_ != layer)
                continue;
            auto collider_component = obj->get_first_component_of_type<
                component::ColliderComponent>();

            if (collider_component == nullptr)
                continue;

            if (!obj->is_active)
                continue;

            glm::mat4 model = obj->get_transform().get_model_matrix();
            glm::mat4 InvModel = glm::inverse(model);
            glm::mat3 normalMatrix = glm::transpose(glm::mat3(InvModel));
            glm::vec3 LocalOrigin = InvModel * glm::vec4(origin, 1);
            glm::vec3 LocalDirection =
                GetNewDirection(origin, LocalOrigin, direction, InvModel);

            // TODO: calculate bounding box intersection

            std::mutex hitMutex;

            std::for_each(
                std::execution::par, collider_component->collider_mesh.begin(),
                collider_component->collider_mesh.end(),
                [&](const auto& objTri) {
                    glm::vec3 localHitPoint;
                    if (!ray_intersects_triangle(LocalOrigin, LocalDirection,
                                                 objTri, localHitPoint))
                        return;

                    glm::vec3 worldHitPoint =
                        model * glm::vec4(localHitPoint, 1);

                    std::lock_guard<std::mutex> lock(hitMutex);
                    if (!hitFound
                        || glm::distance(worldHitPoint, origin)
                            < glm::distance(oldIntersectionPoint, origin))
                    {
                        oldIntersectionPoint = worldHitPoint;
                        if (objOut != nullptr)
                            *objOut = obj;
                        hitFound = true;
                        glm::vec3 localNormal = normalize(
                            cross(objTri.b - objTri.a, objTri.c - objTri.a));
                        out_normal = normalize(normalMatrix * localNormal);
                    }
                });
        }
        out_intersection_point = oldIntersectionPoint;
        if (hitFound)
        {
            graphics::Debug::getInstance()->DrawLine(
                origin, origin + direction * 2000.0f, { 0, 1, 0 }, true);
        }
        else
        {
            graphics::Debug::getInstance()->DrawLine(
                origin, origin + direction * 2000.0f, { 1, 0, 0 }, true);
        }
        return hitFound;
    }

    bool RayCast::FromCamera(glm::vec2 screenPos, RayInfo* OutRayInfo,
                             int layer)
    {
        glm::vec3 direction = GetDirectionFromScreen(screenPos);
        glm::vec3 camPos =
            component::CameraComponent::active_camera->get_position();

        return FromPoint(camPos, direction, OutRayInfo, layer);
    }
    bool RayCast::FromMouse(RayInfo* OutRayInfo, int layer)
    {
        glm::vec2 screenPos = inputs::Mouse::GetMousePos();
#ifdef EDITOR_BUILD
        screenPos.x -= graphics::GraphicApi::viewport_pos_x;
        screenPos.y -= graphics::GraphicApi::viewport_pos_y;
#endif
        return FromCamera(screenPos, OutRayInfo, layer);
    }

    bool RayCast::FromPoint(glm::vec3 origin, glm::vec3 direction,
                            RayInfo* OutRayInfo, int layer)
    {
        Time::StartGlobalTimer();
        static long long mean_time = 0;
        static int hit_count = 0;
        glm::vec3 intersectionPoint;
        glm::vec3 normal;
        objects::GameObject* objOut = nullptr;
        if (haveCollision(origin, direction, intersectionPoint, normal, &objOut,
                          layer))
        {
            OutRayInfo->hitPoint.x = intersectionPoint.x;
            OutRayInfo->hitPoint.y = intersectionPoint.y;
            OutRayInfo->hitPoint.z = intersectionPoint.z;
            OutRayInfo->hitNormal.x = normal.x;
            OutRayInfo->hitNormal.y = normal.y;
            OutRayInfo->hitNormal.z = normal.z;
            OutRayInfo->hitObject = objOut;
            OutRayInfo->hitDistance = glm::distance(origin, intersectionPoint);
            auto calcul_time = Time::StopGlobalTimerAndGet_uS();
            mean_time += calcul_time;
            hit_count++;
            Logger::LogDebug("Resolved raycast with a time of ",
                             std::to_string(calcul_time), " us (",
                             mean_time / hit_count, " us in mean)");
            return true;
        }

        auto calcul_time = Time::StopGlobalTimerAndGet_uS();
        mean_time += calcul_time;
        hit_count++;
        Logger::LogDebug("Failed raycast with a time of ",
                         std::to_string(calcul_time), " us (",
                         mean_time / hit_count, " us in mean)");
        return false;
    }
} // namespace raphEngine