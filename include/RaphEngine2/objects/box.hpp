#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include <glm/glm.hpp>

#include "mesh.hpp"

namespace raphEngine::objects
{
    class RAPHENGINE_API Box
        : public Renderable
        , public Mesh
    {
        void render() override;
        void create_bounding_box(const Mesh& mesh);

        glm::vec3* get_lower_bounds();
        glm::vec3* get_higher_bounds();

    private:
        void calculate_vertexs_();

        bool modified_;
        glm::vec3 lower_bounds_;
        glm::vec3 higher_bounds_;
    };
} // namespace raphEngine::objects
