#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "export.hpp"
#include "renderable.hpp"

namespace raphEngine::objects
{
    struct RAPHENGINE_API MeshInfo
    {
        MeshInfo(const std::string& mesh_path_,
                 bool bilinear_ = true)
            : mesh_path{ mesh_path_ }
            , bilinear{ bilinear_ }
        {}

        std::string mesh_path;
        bool bilinear;
    };
} // namespace raphEngine::objects
