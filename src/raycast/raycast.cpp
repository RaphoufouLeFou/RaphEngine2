#include <execution>
#include <numeric>
#include <limits>
#include <cmath>
#include <algorithm>
#include <objects/game_object.hpp>
#include "RaphEngine2/raycast/raycast.hpp"
#include "component/camera_component.hpp"
#include "component/collider_component.hpp"
#include "graphics/debug.hpp"
#include "graphics/graphic_api.hpp"
#include "inputs/mouse.hpp"
#include "logger/logger.hpp"
#include "time_utils.hpp"
#include "utils.hpp"

#if defined(__AVX2__)
#    include <immintrin.h>
#endif

namespace raphEngine
{

    glm::vec3 GetDirectionFromScreen(glm::vec2 screenPos)
    {
        float ndcX = (2.0f * screenPos.x) / graphics::GraphicApi::res_x - 1.0f;
        float ndcY = 1.0f - (2.0f * screenPos.y) / graphics::GraphicApi::res_y;

        glm::vec4 clipCoords(ndcX, ndcY, -1.0f, 1.0f);

        glm::vec4 eyeCoords =
            inverse(
                component::CameraComponent::active_camera->projection_matrix_)
            * clipCoords;
        eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0, 0.0);

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
                                 glm::vec3& out_intersection_point,
                                 float& out_t)
    {
        constexpr float epsilon = std::numeric_limits<float>::epsilon();

        glm::vec3 edge1 = tri.b - tri.a;
        glm::vec3 edge2 = tri.c - tri.a;
        glm::vec3 ray_cross_e2 = cross(ray_vector, edge2);
        float det = dot(edge1, ray_cross_e2);

        if (det > -epsilon && det < epsilon)
            return false;

        float inv_det = 1.0f / det;
        glm::vec3 s = ray_origin - tri.a;
        float u = inv_det * dot(s, ray_cross_e2);

        if (u < -epsilon || u > 1.0f + epsilon)
            return false;

        glm::vec3 s_cross_e1 = cross(s, edge1);
        float v = inv_det * dot(ray_vector, s_cross_e1);

        if (v < -epsilon || u + v > 1.0f + epsilon)
            return false;

        float t = inv_det * dot(edge2, s_cross_e1);

        if (t > epsilon)
        {
            out_t = t;
            out_intersection_point = glm::vec3(ray_origin + ray_vector * t);
            return true;
        }
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

    // Slab-method ray/AABB test, run in the SAME local space as the collider's
    // triangles. Cheap broad-phase reject before touching a single triangle.
    bool RayIntersectsAABB(const glm::vec3& localOrigin,
                           const glm::vec3& localDirection,
                           const glm::vec3& boundsMin,
                           const glm::vec3& boundsMax, float& tEntryOut)
    {
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();

        for (int axis = 0; axis < 3; axis++)
        {
            float o = localOrigin[axis];
            float d = localDirection[axis];
            float lo = boundsMin[axis];
            float hi = boundsMax[axis];

            if (std::abs(d) < 1e-8f)
            {
                if (o < lo || o > hi)
                    return false; // parallel to this slab and outside it
                continue;
            }

            float invD = 1.0f / d;
            float t1 = (lo - o) * invD;
            float t2 = (hi - o) * invD;
            if (t1 > t2)
                std::swap(t1, t2);

            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax)
                return false;
        }

        tEntryOut = tMin;
        return true;
    }

    glm::vec3 GetNewDirection(glm::vec3 origin, glm::vec3 LocalOrigin,
                              glm::vec3 direction, const glm::mat4& InvModel)
    {
        glm::vec3 DirectionPoint = origin + direction;
        glm::vec3 TransformedDirPoint =
            glm::vec3(InvModel * glm::vec4(DirectionPoint, 1));
        glm::vec3 NewDirection = TransformedDirPoint - LocalOrigin;
        return glm::normalize(NewDirection);
    }

    // Result of finding the closest triangle hit within one object's collider.
    struct RayHit
    {
        bool valid = false;
        float t = std::numeric_limits<float>::infinity();
        Utils::Triangle tri{};
    };

#if defined(__AVX2__)
    struct BatchHit
    {
        int triIndex = -1;
        float t = std::numeric_limits<float>::infinity();
    };

    // Batched Moller-Trumbore over 8 triangles at once. `ox..dz` broadcast the
    // single ray across all 8 lanes; each lane tests one triangle.
    static inline BatchHit
    RayIntersectTriangles8(const component::TriangleSOA& soa, size_t start,
                           __m256 ox, __m256 oy, __m256 oz, __m256 dx,
                           __m256 dy, __m256 dz)
    {
        const __m256 eps =
            _mm256_set1_ps(std::numeric_limits<float>::epsilon());
        const __m256 one = _mm256_set1_ps(1.0f);
        const __m256 zero = _mm256_setzero_ps();
        const __m256 signMask = _mm256_set1_ps(-0.0f);

        __m256 ax = _mm256_loadu_ps(&soa.ax[start]);
        __m256 ay = _mm256_loadu_ps(&soa.ay[start]);
        __m256 az = _mm256_loadu_ps(&soa.az[start]);
        __m256 bx = _mm256_loadu_ps(&soa.bx[start]);
        __m256 by = _mm256_loadu_ps(&soa.by[start]);
        __m256 bz = _mm256_loadu_ps(&soa.bz[start]);
        __m256 cx = _mm256_loadu_ps(&soa.cx[start]);
        __m256 cy = _mm256_loadu_ps(&soa.cy[start]);
        __m256 cz = _mm256_loadu_ps(&soa.cz[start]);

        __m256 e1x = _mm256_sub_ps(bx, ax), e1y = _mm256_sub_ps(by, ay),
               e1z = _mm256_sub_ps(bz, az);
        __m256 e2x = _mm256_sub_ps(cx, ax), e2y = _mm256_sub_ps(cy, ay),
               e2z = _mm256_sub_ps(cz, az);

        // ray_cross_e2 = dir x edge2
        __m256 rx =
            _mm256_sub_ps(_mm256_mul_ps(dy, e2z), _mm256_mul_ps(dz, e2y));
        __m256 ry =
            _mm256_sub_ps(_mm256_mul_ps(dz, e2x), _mm256_mul_ps(dx, e2z));
        __m256 rz =
            _mm256_sub_ps(_mm256_mul_ps(dx, e2y), _mm256_mul_ps(dy, e2x));

        __m256 det = _mm256_add_ps(
            _mm256_add_ps(_mm256_mul_ps(e1x, rx), _mm256_mul_ps(e1y, ry)),
            _mm256_mul_ps(e1z, rz));

        __m256 absDet =
            _mm256_andnot_ps(signMask, det); // clear sign bit -> |det|
        __m256 validDet = _mm256_cmp_ps(absDet, eps, _CMP_GT_OQ);

        // Avoid dividing by ~0 on rejected lanes (keeps NaN/Inf out entirely,
        // even though those lanes get masked out below anyway).
        __m256 safeDet = _mm256_blendv_ps(one, det, validDet);
        __m256 invDet = _mm256_div_ps(one, safeDet);

        __m256 sx = _mm256_sub_ps(ox, ax), sy = _mm256_sub_ps(oy, ay),
               sz = _mm256_sub_ps(oz, az);
        __m256 u =
            _mm256_mul_ps(invDet,
                          _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(sx, rx),
                                                      _mm256_mul_ps(sy, ry)),
                                        _mm256_mul_ps(sz, rz)));

        __m256 validU = _mm256_and_ps(
            _mm256_cmp_ps(u, _mm256_sub_ps(zero, eps), _CMP_GE_OQ),
            _mm256_cmp_ps(u, _mm256_add_ps(one, eps), _CMP_LE_OQ));

        // s_cross_e1 = s x edge1
        __m256 qx =
            _mm256_sub_ps(_mm256_mul_ps(sy, e1z), _mm256_mul_ps(sz, e1y));
        __m256 qy =
            _mm256_sub_ps(_mm256_mul_ps(sz, e1x), _mm256_mul_ps(sx, e1z));
        __m256 qz =
            _mm256_sub_ps(_mm256_mul_ps(sx, e1y), _mm256_mul_ps(sy, e1x));

        __m256 v =
            _mm256_mul_ps(invDet,
                          _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(dx, qx),
                                                      _mm256_mul_ps(dy, qy)),
                                        _mm256_mul_ps(dz, qz)));

        __m256 uv = _mm256_add_ps(u, v);
        __m256 validV = _mm256_and_ps(
            _mm256_cmp_ps(v, _mm256_sub_ps(zero, eps), _CMP_GE_OQ),
            _mm256_cmp_ps(uv, _mm256_add_ps(one, eps), _CMP_LE_OQ));

        __m256 t =
            _mm256_mul_ps(invDet,
                          _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(e2x, qx),
                                                      _mm256_mul_ps(e2y, qy)),
                                        _mm256_mul_ps(e2z, qz)));
        __m256 validT = _mm256_cmp_ps(t, eps, _CMP_GT_OQ);

        __m256 mask = _mm256_and_ps(_mm256_and_ps(validDet, validU),
                                    _mm256_and_ps(validV, validT));

        __m256 inf = _mm256_set1_ps(std::numeric_limits<float>::infinity());
        __m256 maskedT = _mm256_blendv_ps(inf, t, mask);

        alignas(32) float tVals[8];
        _mm256_store_ps(tVals, maskedT);

        BatchHit best;
        for (int lane = 0; lane < 8; lane++)
        {
            if (tVals[lane] < best.t)
            {
                best.t = tVals[lane];
                best.triIndex = static_cast<int>(start + lane);
            }
        }
        return best;
    }
#endif

    // Closest triangle hit for one object's collider, in that object's local
    // space. Uses AVX2 8-wide batches when available, otherwise falls back to
    // a scalar loop -- both parallelized across triangles/batches with
    // transform_reduce (no locks: each task keeps its own local best, and the
    // reduction just picks the smallest t).
    RayHit
    RayIntersectClosestInCollider(const component::ColliderComponent& collider,
                                  const glm::vec3& localOrigin,
                                  const glm::vec3& localDirection)
    {
#if defined(__AVX2__)
        const auto& soa = collider.collider_mesh_soa;
        if (soa.batch_starts.empty())
            return {};

        __m256 ox = _mm256_set1_ps(localOrigin.x);
        __m256 oy = _mm256_set1_ps(localOrigin.y);
        __m256 oz = _mm256_set1_ps(localOrigin.z);
        __m256 dx = _mm256_set1_ps(localDirection.x);
        __m256 dy = _mm256_set1_ps(localDirection.y);
        __m256 dz = _mm256_set1_ps(localDirection.z);

        BatchHit best = std::transform_reduce(
            std::execution::par, soa.batch_starts.begin(),
            soa.batch_starts.end(), BatchHit{},
            [](const BatchHit& a, const BatchHit& b) {
                return a.t < b.t ? a : b;
            },
            [&](size_t start) {
                return RayIntersectTriangles8(soa, start, ox, oy, oz, dx, dy,
                                              dz);
            });

        if (best.triIndex < 0)
            return {};

        return RayHit{ true, best.t, collider.collider_mesh[best.triIndex] };
#else
        const auto& tris = collider.collider_mesh;

        return std::transform_reduce(
            std::execution::par, tris.begin(), tris.end(), RayHit{},
            [](const RayHit& a, const RayHit& b) { return a.t < b.t ? a : b; },
            [&](const Utils::Triangle& tri) -> RayHit {
                glm::vec3 hitPoint;
                float t = 0.0f;
                if (!ray_intersects_triangle(localOrigin, localDirection, tri,
                                             hitPoint, t))
                    return {};
                return RayHit{ true, t, tri };
            });
#endif
    }

    void RayCast::DebugUpdate()
    {}

    bool RayCast::haveCollision(glm::vec3 origin, glm::vec3 direction,
                                glm::vec3& out_intersection_point,
                                glm::vec3& out_normal,
                                objects::GameObject** objOut, int layer)
    {
        glm::vec3 closestPoint(0.0f);
        glm::vec3 closestNormal(0.0f);
        objects::GameObject* closestObj = nullptr;
        float closestDistSq = std::numeric_limits<float>::max();
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
            if (collider_component->collider_mesh.empty())
                continue;

            glm::mat4 model = obj->get_transform().get_model_matrix();
            collider_component->update_cached_transform(model);
            const glm::mat4& InvModel = collider_component->cached_inv_model_;
            const glm::mat3& normalMatrix =
                collider_component->cached_normal_matrix_;

            glm::vec3 LocalOrigin =
                glm::vec3(InvModel * glm::vec4(origin, 1.0f));
            glm::vec3 LocalDirection =
                GetNewDirection(origin, LocalOrigin, direction, InvModel);

            // Broad phase: skip the whole object if the ray misses its AABB.
            float aabbEntryT;
            if (!RayIntersectsAABB(LocalOrigin, LocalDirection,
                                   collider_component->bounding_min,
                                   collider_component->bounding_max,
                                   aabbEntryT))
                continue;

            RayHit hit = RayIntersectClosestInCollider(
                *collider_component, LocalOrigin, LocalDirection);
            if (!hit.valid)
                continue;

            glm::vec3 localHitPoint = LocalOrigin + LocalDirection * hit.t;
            glm::vec3 worldHitPoint =
                glm::vec3(model * glm::vec4(localHitPoint, 1.0f));

            glm::vec3 toHit = worldHitPoint - origin;
            float distSq =
                glm::dot(toHit, toHit); // avoid sqrt per-object comparison

            if (!hitFound || distSq < closestDistSq)
            {
                glm::vec3 localNormal = normalize(
                    cross(hit.tri.b - hit.tri.a, hit.tri.c - hit.tri.a));

                closestPoint = worldHitPoint;
                closestNormal = normalize(normalMatrix * localNormal);
                closestObj = obj;
                closestDistSq = distSq;
                hitFound = true;
            }
        }

        out_intersection_point = closestPoint;
        out_normal = closestNormal;
        if (objOut != nullptr)
            *objOut = closestObj;

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
            Logger::LogDebug("Resolved raycast with a time of ", calcul_time,
                             " us (", mean_time / hit_count, " us in mean)");
            return true;
        }

        auto calcul_time = Time::StopGlobalTimerAndGet_uS();
        mean_time += calcul_time;
        hit_count++;
        Logger::LogDebug("Failed raycast with a time of ", calcul_time, " us (",
                         mean_time / hit_count, " us in mean)");
        return false;
    }
} // namespace raphEngine