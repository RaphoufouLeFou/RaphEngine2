#pragma once

#include <glm/glm.hpp>

#include <RaphEngine2/export.hpp>
#include "mesh.hpp"
#include <RaphEngine2/renderable.hpp>

namespace raphEngine::objects
{
    class RAPHENGINE_API Box : public Renderable, public Mesh
    {
        void render() override;
    private:
        void calculate_vertexs();

        glm::vec3* get_lower_bounds();
        glm::vec3* get_higher_bounds();

        bool modified_;
        glm::vec3 lower_bounds_;
        glm::vec3 higher_bounds_;
    };
} // namespace raphEngine
