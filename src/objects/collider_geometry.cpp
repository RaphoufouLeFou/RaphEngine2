// collider_geometry.cpp
#include "objects/collider_geometry.hpp"
#include "objects/object_mesh.hpp"
#include "objects/mesh.hpp"

namespace raphEngine::component
{
    std::unordered_map<ColliderSourceKey, std::weak_ptr<const ColliderGeometry>,
                       ColliderSourceKeyHash>
        ColliderGeometryCache::cache_;

    static void add_tri(const objects::Mesh* mesh,
                        std::vector<Utils::Triangle>& out)
    {
        const glm::mat4 meshModel = mesh->get_model_matrix();
        const auto& verts = mesh->get_vertices();
        const auto& indices = mesh->get_indices();
        bool isIndexed = !indices.empty();
        size_t triSource = isIndexed ? indices.size() : verts.size();
        int triCount = static_cast<int>(triSource / 3);

        out.reserve(out.size() + triCount);
        for (int k = 0; k < triCount; k++)
        {
            unsigned int i0, i1, i2;
            if (isIndexed)
            {
                i0 = indices[k * 3];
                i1 = indices[k * 3 + 1];
                i2 = indices[k * 3 + 2];
            }
            else
            {
                i0 = static_cast<unsigned int>(k * 3);
                i1 = static_cast<unsigned int>(k * 3 + 1);
                i2 = static_cast<unsigned int>(k * 3 + 2);
            }
            Utils::Triangle t;
            t.a = glm::vec3(meshModel * glm::vec4(verts[i0].position, 1.0f));
            t.b = glm::vec3(meshModel * glm::vec4(verts[i1].position, 1.0f));
            t.c = glm::vec3(meshModel * glm::vec4(verts[i2].position, 1.0f));
            out.push_back(t);
        }
    }

    static void build_bounds(ColliderGeometry& geo)
    {
        if (geo.triangles.empty())
        {
            geo.bounding_min = geo.bounding_max = glm::vec3{ 0 };
            return;
        }
        geo.bounding_min = geo.bounding_max = geo.triangles[0].a;
        for (const auto& t : geo.triangles)
            for (const auto& p : { t.a, t.b, t.c })
            {
                geo.bounding_min = glm::min(geo.bounding_min, p);
                geo.bounding_max = glm::max(geo.bounding_max, p);
            }
    }

    static void build_soa(ColliderGeometry& geo)
    {
#if defined(__AVX2__)
        size_t count = geo.triangles.size();
        size_t padded = ((count + 7) / 8) * 8;
        auto& soa = geo.soa;
        soa.ax.assign(padded, 0.0f);
        soa.ay.assign(padded, 0.0f);
        soa.az.assign(padded, 0.0f);
        soa.bx.assign(padded, 0.0f);
        soa.by.assign(padded, 0.0f);
        soa.bz.assign(padded, 0.0f);
        soa.cx.assign(padded, 0.0f);
        soa.cy.assign(padded, 0.0f);
        soa.cz.assign(padded, 0.0f);

        for (size_t k = 0; k < count; k++)
        {
            const auto& t = geo.triangles[k];
            soa.ax[k] = t.a.x;
            soa.ay[k] = t.a.y;
            soa.az[k] = t.a.z;
            soa.bx[k] = t.b.x;
            soa.by[k] = t.b.y;
            soa.bz[k] = t.b.z;
            soa.cx[k] = t.c.x;
            soa.cy[k] = t.c.y;
            soa.cz[k] = t.c.z;
        }
        soa.count = count;
        soa.paddedCount = padded;
        size_t numBatches = padded / 8;
        soa.batch_starts.resize(numBatches);
        for (size_t b = 0; b < numBatches; b++)
            soa.batch_starts[b] = b * 8;
#endif
    }

    std::shared_ptr<const ColliderGeometry>
    ColliderGeometryCache::get_or_build(const objects::ObjectMesh* lod0)
    {
        ColliderSourceKey key;
        key.parts.reserve(lod0->meshes_.size());
        for (const auto& m : lod0->meshes_)
            key.parts.push_back(m->get_source_key());

        if (auto it = cache_.find(key); it != cache_.end())
            if (auto locked = it->second.lock())
                return locked;

        auto geo = std::make_shared<ColliderGeometry>();
        for (const auto& m : lod0->meshes_)
            add_tri(m.get(), geo->triangles);
        build_bounds(*geo);
        build_soa(*geo);

        cache_[key] = geo;
        return geo;
    }
} // namespace raphEngine::component