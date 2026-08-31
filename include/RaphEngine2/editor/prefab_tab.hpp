#pragma once

#include <RaphEngine2/export.hpp>
#include <filesystem>
#include "objects/game_object.hpp"

namespace raphEngine::editor
{
    namespace fs = std::filesystem;

    class RAPHENGINE_API PrefabTab
    {
    public:
        static void Init();
        static void Update();
        static void SetPrefabsPath(const fs::path& paf);
        static const fs::path& GetPrefabsPath();
        static void save_as_prefab(const objects::GameObject* object);
        static void load_prefab(const std::string& name);

    private:
        static fs::path prefab_folder;

        static void load_prefabs_json();
        static void display_json();
    };
} // namespace raphEngine::editor
