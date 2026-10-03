#pragma once

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/chunk.hpp>
#include <RaphEngine2/terrain/map.hpp>
#include <RaphEngine2/terrain/material_generator.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

namespace raphEngine::objects
{
    class GameObject;
}

namespace raphEngine::terrain
{
    // One placeable tree type. TreeRecord::speciesId indexes into whatever
    // species table the VegetationManager is constructed with.
    struct TreeSpecies
    {
        std::string prefabName; // passed straight to GameObject::instanciate
    };

    struct TreePlacementParams
    {
        uint32_t seed = 1337;
        float cellSizeMeters = 8.0f; // jittered-grid cell size
        float maxSlopeDegrees = 35.0f; // steeper than this: no tree
        float maxRockWeight = 0.5f; // too much bare rock: no tree
        float maxSnowWeight = 0.4f; // too much snow: no tree
        float densityThreshold = 0.45f; // forest-noise cutoff (0..1)
        float minScale = 0.8f;
        float maxScale = 1.4f;
    };

    // Deterministic given (gridCoord, the map's height/material data,
    // params, speciesCount): same inputs always produce the same candidate
    // list in the same order. That's what lets Chunk's removed-candidate
    // index set stay meaningful across reloads — changing params or the
    // terrain under a chunk invalidates its existing overlay's meaning,
    // same caveat as changing terrain generation parameters already has.
    //
    // Pure function, no side effects, safe to call speculatively (e.g. from
    // an editor preview) without touching any Chunk state.
    std::vector<std::pair<uint32_t, TreeRecord>>
    GenerateTreeCandidates(glm::ivec2 gridCoord, const Map& map,
                           const MaterialGenerator& materialGenerator,
                           const TreePlacementParams& params,
                           uint16_t speciesCount);

    // Owns the live tree GameObjects for all currently-resident chunks.
    //
    // Wiring:
    //   - Call OnChunkLoaded/OnChunkUnloaded from Map::UpdateStreaming's
    //     onChunkLoaded/onChunkUnloaded callbacks (or forward them from
    //     wherever your game loop already calls UpdateStreaming).
    //   - Call TryRemoveTree with whatever GameObject* your raycast/picking
    //     code hits; it returns false (does nothing) if that object isn't a
    //     vegetation-managed tree, so it's safe to call unconditionally.
    //   - Call AddTreeAt to place a new tree, e.g. at a raycast hit point.
    class RAPHENGINE_API VegetationManager
    {
    public:
        VegetationManager(Map& map, TreePlacementParams params,
                          std::vector<TreeSpecies> species);

        void OnChunkLoaded(glm::ivec2 gridCoord, Chunk& chunk);
        void OnChunkUnloaded(glm::ivec2 gridCoord, Chunk& chunk);

        bool TryRemoveTree(objects::GameObject* hitObject);

        // worldPosition.z is ignored/overwritten with the real terrain
        // height at that XY, so callers don't need to pre-resolve ground
        // height themselves — just pass the XY you want (e.g. a raycast
        // hit's X/Y). Returns nullptr if worldPosition doesn't fall inside
        // a currently-resident chunk.
        objects::GameObject* AddTreeAt(glm::vec3 worldPosition,
                                       float rotationYDegrees, float scale,
                                       uint16_t speciesId);

    private:
        struct TreeInstanceTag
        {
            glm::ivec2 gridCoord{ 0, 0 };
            uint32_t index = 0; // candidateIndex if !isAdded, else addedTreeId
            bool isAdded = false;
        };

        objects::GameObject* SpawnTree(const TreeRecord& record,
                                       glm::ivec2 gridCoord, uint32_t index,
                                       bool isAdded);
        void DespawnChunkTrees(glm::ivec2 gridCoord);

        Map& map_;
        TreePlacementParams params_;
        std::vector<TreeSpecies> species_;
        MaterialGenerator materialGenerator_;

        std::unordered_map<objects::GameObject*, TreeInstanceTag> instanceTags_;
        std::unordered_map<uint64_t, std::vector<objects::GameObject*>>
            chunkInstances_;
    };
} // namespace raphEngine::terrain
