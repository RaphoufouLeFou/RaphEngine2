#pragma once

#include <RaphEngine2/export.hpp>
#include <cstddef>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace raphEngine::objects
{
    class Mesh;
}

namespace raphEngine::graphics
{
    struct MeshSourceKey
    {
        const void* resource;
        size_t submesh_index;
        bool operator==(const MeshSourceKey&) const = default;
    };

    struct MeshSourceKeyHash
    {
        size_t operator()(const MeshSourceKey& k) const
        {
            size_t h = std::hash<const void*>{}(k.resource);
            h ^= std::hash<size_t>{}(k.submesh_index) + 0x9e3779b9 + (h << 6)
                + (h >> 2);
            return h;
        }
    };

    class RAPHENGINE_API MeshBuffers
    {
    public:
        virtual ~MeshBuffers() = default;

        virtual void GenerateBuffers() = 0;

        static std::shared_ptr<MeshBuffers> getMeshBuffer(objects::Mesh* mesh);

    private:
        static std::shared_ptr<MeshBuffers>
        CreateBackendBuffers(objects::Mesh* mesh);
        static std::unordered_map<MeshSourceKey, std::weak_ptr<MeshBuffers>,
                                  MeshSourceKeyHash>
            cache_;
    };
} // namespace raphEngine::graphics