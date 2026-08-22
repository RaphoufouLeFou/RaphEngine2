#pragma once

#include <glm/glm.hpp>
#include <string>

#include "RaphEngine2/export.hpp"
#include "scenes/reflection.hpp"

namespace raphEngine::objects
{
    struct RAPHENGINE_API MeshInfo
    {
        MeshInfo() = default;
        MeshInfo(const std::string& mesh_path_, bool bilinear_ = true)
            : mesh_path{ mesh_path_ }
            , bilinear{ bilinear_ }
        {}

        std::string mesh_path;
        bool bilinear;

        friend void to_json(nlohmann::json& j, const MeshInfo& m)
        {
            j = reflection::toJson(m);
        }
        friend void from_json(const nlohmann::json& j, MeshInfo& m)
        {
            reflection::fromJson(m, j);
        }

    private:
        REFLECT_ROOT(MeshInfo, mesh_path, bilinear)
    };

} // namespace raphEngine::objects
