#pragma once

#include <RaphEngine2/utils.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>
#include "RaphEngine2/graphics/mesh_buffers.hpp"
#include "RaphEngine2/objects/object_mesh.hpp"

namespace raphEngine::component
{
    struct RAPHENGINE_API TriangleSOA
    {
        std::vector<float> ax, ay, az, bx, by, bz, cx, cy, cz;
        std::vector<size_t> batch_starts;
        size_t count = 0;
        size_t paddedCount = 0;
    };

    struct RAPHENGINE_API ColliderGeometry
    {
        std::vector<Utils::Triangle> triangles;
        TriangleSOA soa;
        glm::vec3 bounding_min{ 0.0f };
        glm::vec3 bounding_max{ 0.0f };
    };

    struct RAPHENGINE_API ColliderSourceKey
    {
        std::vector<graphics::MeshSourceKey> parts;
        bool operator==(const ColliderSourceKey&) const = default;
    };

    struct RAPHENGINE_API ColliderSourceKeyHash
    {
        size_t operator()(const ColliderSourceKey& k) const
        {
            graphics::MeshSourceKeyHash sub_hash;
            size_t h = k.parts.size();
            for (const auto& p : k.parts)
                h ^= sub_hash(p) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    class RAPHENGINE_API ColliderGeometryCache
    {
    public:
        static std::shared_ptr<const ColliderGeometry>
        get_or_build(const objects::ObjectMesh* lod0);

    private:
        static std::unordered_map<ColliderSourceKey,
                                  std::weak_ptr<const ColliderGeometry>,
                                  ColliderSourceKeyHash>
            cache_;
    };
} // namespace raphEngine::component