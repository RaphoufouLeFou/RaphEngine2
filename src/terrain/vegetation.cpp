#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/vegetation.hpp>

#include <RaphEngine2/objects/game_object.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace raphEngine::terrain
{
    namespace
    {
        // Self-contained deterministic 2D value noise with a couple of fBm
        // octaves for forest clumping. Deliberately independent of
        // terrain::PerlinNoise/SampleFractalNoise so this file has no
        // dependency on that class's exact API — it only needs organic
        // clumping, not pixel-identical noise to whatever the terrain's own
        // dirt-patch generation uses. Swapping to the engine's shared noise
        // utilities later is a drop-in replacement if that cross-system
        // visual consistency ever matters.
        uint32_t HashUint(uint32_t x)
        {
            x ^= x >> 16;
            x *= 0x7feb352dU;
            x ^= x >> 15;
            x *= 0x846ca68bU;
            x ^= x >> 16;
            return x;
        }

        float HashToUnitFloat(glm::ivec2 cell, uint32_t seed)
        {
            const uint32_t h =
                HashUint(static_cast<uint32_t>(cell.x) * 0x1f1f1f1fU
                         ^ static_cast<uint32_t>(cell.y) ^ seed);
            return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFU);
        }

        float ValueNoise2D(glm::vec2 pos, uint32_t seed)
        {
            const glm::vec2 base = glm::floor(pos);
            const glm::vec2 frac = pos - base;
            const glm::ivec2 cell(base);

            const float h00 = HashToUnitFloat(cell + glm::ivec2(0, 0), seed);
            const float h10 = HashToUnitFloat(cell + glm::ivec2(1, 0), seed);
            const float h01 = HashToUnitFloat(cell + glm::ivec2(0, 1), seed);
            const float h11 = HashToUnitFloat(cell + glm::ivec2(1, 1), seed);

            const glm::vec2 smooth =
                frac * frac * (glm::vec2(3.0f) - 2.0f * frac);

            const float top = glm::mix(h00, h10, smooth.x);
            const float bottom = glm::mix(h01, h11, smooth.x);
            return glm::mix(top, bottom, smooth.y);
        }

        float ForestDensityNoise(glm::vec2 worldXY, uint32_t seed)
        {
            float value = 0.0f;
            float amplitude = 0.5f;
            float frequency = 1.0f / 180.0f; // ~180m base clump size
            float amplitudeSum = 0.0f;

            for (int octave = 0; octave < 3; ++octave)
            {
                value +=
                    ValueNoise2D(worldXY * frequency,
                                 seed + static_cast<uint32_t>(octave) * 101u)
                    * amplitude;
                amplitudeSum += amplitude;
                amplitude *= 0.5f;
                frequency *= 2.3f;
            }

            return value / amplitudeSum;
        }

        uint64_t PackGridCoord(glm::ivec2 coord)
        {
            return (static_cast<uint64_t>(static_cast<uint32_t>(coord.x)) << 32)
                | static_cast<uint64_t>(static_cast<uint32_t>(coord.y));
        }
    } // namespace

    std::vector<std::pair<uint32_t, TreeRecord>>
    GenerateTreeCandidates(glm::ivec2 gridCoord, const Map& map,
                           const MaterialGenerator& materialGenerator,
                           const TreePlacementParams& params,
                           uint16_t speciesCount)
    {
        std::vector<std::pair<uint32_t, TreeRecord>> candidates;

        if (speciesCount == 0 || params.cellSizeMeters <= 0.0f)
        {
            return candidates;
        }

        const glm::vec2 worldOrigin = map.GetChunkWorldOrigin(gridCoord);
        const uint32_t cellsPerAxis = static_cast<uint32_t>(
            static_cast<float>(kChunkResolution) / params.cellSizeMeters);

        const uint32_t chunkSeed = params.seed
            ^ (static_cast<uint32_t>(gridCoord.x) * 0x9E3779B1u)
            ^ (static_cast<uint32_t>(gridCoord.y) * 0x85EBCA77u);

        for (uint32_t cellY = 0; cellY < cellsPerAxis; ++cellY)
        {
            for (uint32_t cellX = 0; cellX < cellsPerAxis; ++cellX)
            {
                const uint32_t candidateIndex = cellY * cellsPerAxis + cellX;
                const glm::ivec2 cell(static_cast<int>(cellX),
                                      static_cast<int>(cellY));

                // Deterministic jitter within the cell so the result isn't
                // a visibly uniform grid.
                const float jitterX =
                    HashToUnitFloat(cell, chunkSeed ^ 0x1111u);
                const float jitterY =
                    HashToUnitFloat(cell, chunkSeed ^ 0x2222u);

                const glm::vec2 localPosition(
                    (static_cast<float>(cellX) + jitterX)
                        * params.cellSizeMeters,
                    (static_cast<float>(cellY) + jitterY)
                        * params.cellSizeMeters);

                const glm::vec2 worldXY = worldOrigin + localPosition;
                const float height = map.GetHeightAt(worldXY);

                // Slope via finite differences — same general approach the
                // material/shading systems already use for consistency with
                // what's actually visible on the ground here.
                constexpr float kSlopeSampleDistance = 2.0f;
                const float heightX = map.GetHeightAt(
                    worldXY + glm::vec2(kSlopeSampleDistance, 0.0f));
                const float heightY = map.GetHeightAt(
                    worldXY + glm::vec2(0.0f, kSlopeSampleDistance));

                const glm::vec3 tangentX(kSlopeSampleDistance, 0.0f,
                                         heightX - height);
                const glm::vec3 tangentY(0.0f, kSlopeSampleDistance,
                                         heightY - height);
                const glm::vec3 normal =
                    glm::normalize(glm::cross(tangentX, tangentY));
                const float slopeDegrees =
                    glm::degrees(glm::acos(glm::clamp(normal.z, -1.0f, 1.0f)));

                if (slopeDegrees > params.maxSlopeDegrees)
                {
                    continue;
                }

                const MaterialWeights weights =
                    materialGenerator.ComputeWeightsAt(
                        worldXY,
                        [&map](glm::vec2 xy) { return map.GetHeightAt(xy); });

                const float rockWeight =
                    static_cast<float>(weights.rock) / 255.0f;
                const float snowWeight =
                    static_cast<float>(weights.snow) / 255.0f;
                if (rockWeight > params.maxRockWeight
                    || snowWeight > params.maxSnowWeight)
                {
                    continue;
                }

                const float forestDensity =
                    ForestDensityNoise(worldXY, params.seed);
                if (forestDensity < params.densityThreshold)
                {
                    continue;
                }

                // How far past the threshold we are maps to a placement
                // probability, so forest cover thins out gradually at its
                // edges instead of cutting off in a hard line.
                const float densityRange =
                    std::max(0.001f, 1.0f - params.densityThreshold);
                const float placementChance = glm::clamp(
                    (forestDensity - params.densityThreshold) / densityRange,
                    0.0f, 1.0f);
                const float roll = HashToUnitFloat(cell, chunkSeed ^ 0x3333u);
                if (roll > placementChance)
                {
                    continue;
                }

                TreeRecord record{};
                record.localPosition = localPosition;
                record.rotationY =
                    HashToUnitFloat(cell, chunkSeed ^ 0x4444u) * 360.0f;
                record.scale =
                    glm::mix(params.minScale, params.maxScale,
                             HashToUnitFloat(cell, chunkSeed ^ 0x5555u));
                record.speciesId = static_cast<uint16_t>(
                    HashToUnitFloat(cell, chunkSeed ^ 0x6666u)
                    * static_cast<float>(speciesCount));
                if (record.speciesId >= speciesCount)
                {
                    record.speciesId = speciesCount - 1;
                }

                candidates.emplace_back(candidateIndex, record);
            }
        }

        return candidates;
    }

    VegetationManager::VegetationManager(Map& map, TreePlacementParams params,
                                         std::vector<TreeSpecies> species)
        : map_(map)
        , params_(std::move(params))
        , species_(std::move(species))
        , materialGenerator_(params_.seed, map.GetHeightRange())
    {}

    objects::GameObject* VegetationManager::SpawnTree(const TreeRecord& record,
                                                      glm::ivec2 gridCoord,
                                                      uint32_t index,
                                                      bool isAdded)
    {
        if (record.speciesId >= species_.size())
        {
            return nullptr;
        }

        objects::GameObject* go = objects::GameObject::instanciate(
            species_[record.speciesId].prefabName);
        if (!go)
        {
            return nullptr;
        }

        const glm::vec2 worldOrigin = map_.GetChunkWorldOrigin(gridCoord);
        const glm::vec2 worldXY = worldOrigin + record.localPosition;
        const float worldHeight = map_.GetHeightAt(worldXY);

        objects::Transform& transform = go->get_transform();
        transform.set_position(glm::vec3(worldXY, worldHeight));
        transform.set_rotation(glm::vec3(0.0f, 0.0f, record.rotationY));
        transform.set_scale(glm::vec3(record.scale));
        transform.can_have_moved = true;

        instanceTags_.emplace(go, TreeInstanceTag{ gridCoord, index, isAdded });
        return go;
    }

    void VegetationManager::OnChunkLoaded(glm::ivec2 gridCoord, Chunk& chunk)
    {
        const std::vector<std::pair<uint32_t, TreeRecord>> candidates =
            GenerateTreeCandidates(gridCoord, map_, materialGenerator_, params_,
                                   static_cast<uint16_t>(species_.size()));

        std::vector<objects::GameObject*> spawned;
        spawned.reserve(candidates.size() + chunk.GetAddedTrees().size());

        for (const auto& [candidateIndex, record] : candidates)
        {
            if (chunk.IsTreeCandidateRemoved(candidateIndex))
            {
                continue;
            }

            objects::GameObject* go =
                SpawnTree(record, gridCoord, candidateIndex, false);
            if (go)
            {
                spawned.push_back(go);
            }
        }

        for (const auto& [addedTreeId, record] : chunk.GetAddedTrees())
        {
            objects::GameObject* go =
                SpawnTree(record, gridCoord, addedTreeId, true);
            if (go)
            {
                spawned.push_back(go);
            }
        }

        chunkInstances_[PackGridCoord(gridCoord)] = std::move(spawned);
    }

    void VegetationManager::OnChunkUnloaded(glm::ivec2 gridCoord, Chunk&)
    {
        DespawnChunkTrees(gridCoord);
    }

    void VegetationManager::DespawnChunkTrees(glm::ivec2 gridCoord)
    {
        const auto it = chunkInstances_.find(PackGridCoord(gridCoord));
        if (it == chunkInstances_.end())
        {
            return;
        }

        for (objects::GameObject* go : it->second)
        {
            instanceTags_.erase(go);
            objects::GameObject::destroy(*go);
        }

        chunkInstances_.erase(it);
    }

    bool VegetationManager::TryRemoveTree(objects::GameObject* hitObject)
    {
        if (!hitObject)
        {
            return false;
        }

        const auto tagIt = instanceTags_.find(hitObject);
        if (tagIt == instanceTags_.end())
        {
            return false; // not a vegetation-managed object
        }

        const TreeInstanceTag tag = tagIt->second;

        Chunk* chunk = map_.GetChunkAt(tag.gridCoord);
        if (chunk)
        {
            if (tag.isAdded)
            {
                chunk->RemoveAddedTree(tag.index);
            }
            else
            {
                chunk->RemoveTreeCandidate(tag.index);
            }
        }

        const auto chunkIt = chunkInstances_.find(PackGridCoord(tag.gridCoord));
        if (chunkIt != chunkInstances_.end())
        {
            std::vector<objects::GameObject*>& instances = chunkIt->second;
            instances.erase(
                std::remove(instances.begin(), instances.end(), hitObject),
                instances.end());
        }

        instanceTags_.erase(tagIt);
        objects::GameObject::destroy(*hitObject);
        return true;
    }

    objects::GameObject* VegetationManager::AddTreeAt(glm::vec3 worldPosition,
                                                      float rotationYDegrees,
                                                      float scale,
                                                      uint16_t speciesId)
    {
        const glm::vec2 worldXY(worldPosition.x, worldPosition.y);

        Chunk* chunk = map_.GetChunkContainingWorldPosition(worldXY);
        if (!chunk || chunk->GetState() != ChunkState::Resident)
        {
            return nullptr; // only inside a currently-loaded chunk
        }

        const glm::ivec2 gridCoord = chunk->GetGridCoord();
        const glm::vec2 worldOrigin = map_.GetChunkWorldOrigin(gridCoord);

        TreeRecord record{};
        record.localPosition = worldXY - worldOrigin;
        record.rotationY = rotationYDegrees;
        record.scale = scale;
        record.speciesId = speciesId;

        const uint32_t addedTreeId = chunk->AddTree(record);
        objects::GameObject* go =
            SpawnTree(record, gridCoord, addedTreeId, true);

        if (go)
        {
            chunkInstances_[PackGridCoord(gridCoord)].push_back(go);
        }

        return go;
    }
} // namespace raphEngine::terrain
