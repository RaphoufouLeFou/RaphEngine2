#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/noise_editor.hpp>
#include <RaphEngine2/terrain/map.hpp>

#include <algorithm>
#include <random>
#include <system_error>

#include <imgui.h>

namespace raphEngine::terrain
{
    void DrawNoiseEditorWindow(FractalNoiseParams& params, float sizeInMeters,
                               float height, const fs::path& outputDirectory,
                               NoiseChunkGenerator* liveGenerator, bool* open)
    {
        if (!ImGui::Begin("Terrain noise", open))
        {
            ImGui::End();
            return;
        }

        bool changed = false;

        int seed = static_cast<int>(params.seed);
        if (ImGui::InputInt("Seed", &seed))
        {
            params.seed = static_cast<uint32_t>(seed);
            changed = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Randomize"))
        {
            params.seed = std::random_device{}();
            changed = true;
        }

        changed |= ImGui::SliderFloat("Base feature scale (m)",
                                      &params.baseFeatureScale, 16.0f, 4096.0f,
                                      "%.0f", ImGuiSliderFlags_Logarithmic);
        changed |= ImGui::SliderInt("Octaves", &params.octaves, 1, 10);
        changed |=
            ImGui::SliderFloat("Persistence", &params.persistence, 0.1f, 1.0f);
        changed |=
            ImGui::SliderFloat("Lacunarity", &params.lacunarity, 1.0f, 4.0f);

        if (changed)
        {
            params.octaves = std::max(params.octaves, 1);
            params.baseFeatureScale = std::max(params.baseFeatureScale, 1.0f);

            BuildMapShellFromNoise(params, sizeInMeters, height,
                                   outputDirectory);

            std::error_code ec;
            for (const auto& entry :
                 fs::directory_iterator(outputDirectory, ec))
            {
                const fs::path ext = entry.path().extension();
                if (ext == ".chunk" || ext == ".edits" || ext == ".paint")
                {
                    fs::remove(entry.path(), ec);
                }
            }

            if (liveGenerator)
            {
                liveGenerator->SetParams(params, height);
            }

            if (Map* map = Map::GetInstace())
            {
                map->Load(outputDirectory);
            }
        }

        ImGui::End();
    }
} // namespace raphEngine::terrain
