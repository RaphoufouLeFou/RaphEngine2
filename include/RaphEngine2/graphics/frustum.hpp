#pragma once

#include <cmath>
#include <glm/glm.hpp>

namespace raphEngine::graphics
{
    // 6-plane frustum (orthographic light/cascade). Gribb-Hartmann
    // extraction from a view-projection matrix. Normals point INTO the
    // frustum.
    struct Frustum
    {
        glm::vec4 planes[6];

        static Frustum FromMatrix(const glm::mat4& view_proj)
        {
            Frustum f;
            glm::vec4 row0(view_proj[0][0], view_proj[1][0], view_proj[2][0],
                           view_proj[3][0]);
            glm::vec4 row1(view_proj[0][1], view_proj[1][1], view_proj[2][1],
                           view_proj[3][1]);
            glm::vec4 row2(view_proj[0][2], view_proj[1][2], view_proj[2][2],
                           view_proj[3][2]);
            glm::vec4 row3(view_proj[0][3], view_proj[1][3], view_proj[2][3],
                           view_proj[3][3]);

            f.planes[0] = row3 + row0; // left
            f.planes[1] = row3 - row0; // right
            f.planes[2] = row3 + row1; // bottom
            f.planes[3] = row3 - row1; // top
            f.planes[4] = row3 + row2; // near
            f.planes[5] = row3 - row2; // far

            for (auto& p : f.planes)
            {
                float len = glm::length(glm::vec3(p));
                if (len > 1e-8f)
                    p /= len;
            }
            return f;
        }

        bool IntersectsSphere(const glm::vec3& center, float radius) const
        {
            for (const auto& p : planes)
            {
                float dist =
                    p.x * center.x + p.y * center.y + p.z * center.z + p.w;
                if (dist < -radius)
                    return false;
            }
            return true;
        }
    };

    // Bounding-cone approximation of a perspective camera frustum. Cheaper
    // than the 6-plane test and gives a distance cutoff (range) for free.
    // Conservative: never wrongly culls a visible sphere; may keep a few
    // spheres just outside the true frustum's corners.
    struct Cone
    {
        glm::vec3 apex{ 0.0f };
        glm::vec3 axis{ 0.0f, 0.0f, -1.0f };
        float cos_half_angle = 1.0f;
        float sin_half_angle = 0.0f;
        float range = 0.0f;

        static Cone FromCamera(const glm::vec3& position,
                               const glm::vec3& forward,
                               const glm::mat4& projection, float max_distance)
        {
            Cone c;
            c.apex = position;
            c.axis = glm::normalize(forward);
            c.range = max_distance;

            float tanX = 1.0f / projection[0][0];
            float tanY = 1.0f / projection[1][1];
            float t = std::sqrt(tanX * tanX + tanY * tanY);
            float invLen = 1.0f / std::sqrt(1.0f + t * t);
            c.cos_half_angle = invLen;
            c.sin_half_angle = t * invLen;
            return c;
        }

        bool Intersects(const glm::vec3& center, float radius) const
        {
            glm::vec3 d = center - apex;
            float axialDist = glm::dot(d, axis);

            if (axialDist + radius < 0.0f)
                return false;
            if (axialDist - radius > range)
                return false;

            float distSq = glm::dot(d, d);
            float perpSq = distSq - axialDist * axialDist;
            perpSq = perpSq > 0.0f ? perpSq : 0.0f;
            float perpDist = std::sqrt(perpSq);

            float distToSurface =
                perpDist * cos_half_angle - axialDist * sin_half_angle;
            return distToSurface <= radius;
        }
    };
} // namespace raphEngine::graphics