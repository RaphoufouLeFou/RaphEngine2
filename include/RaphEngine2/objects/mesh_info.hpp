#pragma once

#include <glm/glm.hpp>
#include "RaphEngine2/export.hpp"
#include "RaphEngine2/scenes/reflection.hpp"

#include <filesystem>
namespace fs = std::filesystem;

namespace raphEngine::objects
{
    struct RAPHENGINE_API MeshInfo
    {
        MeshInfo() = default;
        MeshInfo(const fs::path& mesh_path_, bool bilinear_ = true)
            : mesh_path{ mesh_path_ }
            , bilinear{ bilinear_ }
        {}

        fs::path mesh_path;
        bool bilinear = true;

        friend void to_json(nlohmann::json& j, const MeshInfo& m)
        {
            j = nlohmann::json{ { "mesh_path", m.mesh_path },
                                { "bilinear", m.bilinear } };
        }

        friend void from_json(const nlohmann::json& j, MeshInfo& m)
        {
            if (j.contains("mesh_path"))
                m.mesh_path = j.at("mesh_path").get<fs::path>();
            if (j.contains("bilinear"))
                m.bilinear = j.at("bilinear").get<bool>();
        }

    private:
        REFLECT_ROOT(MeshInfo)
    };

} // namespace raphEngine::objects
