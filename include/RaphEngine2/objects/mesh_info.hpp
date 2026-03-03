#pragma once

#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <vector>

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include "shader.hpp"
#include "object_mesh.hpp"

namespace raphEngine::objects
{
    struct RAPHENGINE_API MeshInfo
    {
        
        MeshInfo(const std::string& mesh_path_, std::unique_ptr<Shader> shader_, bool bilinear_)
            : mesh_path {mesh_path_}
            , shader {std::move(shader_)}
            , bilinear {bilinear_}
        {}

        std::string mesh_path;
        std::unique_ptr<Shader> shader;
        bool bilinear;
    };
}
