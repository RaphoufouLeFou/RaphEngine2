#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "export.hpp"
#include "renderable.hpp"
#include "shader.hpp"

namespace raphEngine::objects
{
    struct RAPHENGINE_API MeshInfo
    {
        MeshInfo(const std::string& mesh_path_, std::shared_ptr<Shader> shader_,
                 bool bilinear_ = true)
            : mesh_path{ mesh_path_ }
            , shader{ shader_ }
            , bilinear{ bilinear_ }
        {}

        std::string mesh_path;
        std::shared_ptr<Shader> shader;
        bool bilinear;
    };
} // namespace raphEngine::objects
