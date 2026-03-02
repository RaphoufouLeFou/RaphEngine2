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
    class RAPHENGINE_API MeshInfo
    {
        MeshInfo(const std::string& mesh_path, std::unique_ptr<Shader> shader)
            : mesh_path_ {mesh_path}
            , shader_ {std::move(shader)}
        {}

    private:
        std::string mesh_path_;
        std::unique_ptr<Shader> shader_;
    };
}
