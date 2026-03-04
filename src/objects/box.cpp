#include "objects/box.hpp"

#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>

namespace raphEngine::objects
{
    void Box::render()
    {
        if (modified_)
            calculate_vertexs_();
    }

    void Box::calculate_vertexs_()
    {
        vertices_.reserve(8);
        indices_.reserve(6 * 6);

        /*

              2---------3
             /|        /|
            6---------7 |
            | |       | |
            | 0-------|-1
            |/        |/
            4---------5

        */

        vertices_ = {
            Vertex{ glm::vec3{ lower_bounds_.x, lower_bounds_.y,
                               lower_bounds_.z } },
            Vertex{ glm::vec3{ higher_bounds_.x, lower_bounds_.y,
                               lower_bounds_.z } },
            Vertex{ glm::vec3{ lower_bounds_.x, higher_bounds_.y,
                               lower_bounds_.z } },
            Vertex{ glm::vec3{ higher_bounds_.x, higher_bounds_.y,
                               lower_bounds_.z } },
            Vertex{ glm::vec3{ lower_bounds_.x, lower_bounds_.y,
                               higher_bounds_.z } },
            Vertex{ glm::vec3{ higher_bounds_.x, lower_bounds_.y,
                               higher_bounds_.z } },
            Vertex{ glm::vec3{ lower_bounds_.x, higher_bounds_.y,
                               higher_bounds_.z } },
            Vertex{ glm::vec3{ higher_bounds_.x, higher_bounds_.y,
                               higher_bounds_.z } },
        };

        // magic numbers nice, trust it
        indices_ = {
            0, 1, 4, 4, 1, 5, 6, 7, 4, 4, 7, 5, 0, 2, 3, 0, 3, 1,
            2, 3, 6, 6, 3, 7, 5, 7, 3, 5, 3, 1, 4, 6, 2, 4, 2, 0,
        };
        modified_ = false;
    }

    void Box::create_bounding_box(const Mesh& mesh)
    {
        if (mesh.get_vertices().size() == 0)
        {
            lower_bounds_ = glm::vec3{ 0 };
            higher_bounds_ = glm::vec3{ 0 };
            return;
        }

        lower_bounds_ = mesh.get_vertices().at(0).position;
        higher_bounds_ = lower_bounds_;

        for (const auto& v : mesh.get_vertices())
        {
            if (higher_bounds_.x < v.position.x)
                higher_bounds_.x = v.position.x;
            if (higher_bounds_.y < v.position.y)
                higher_bounds_.y = v.position.y;
            if (higher_bounds_.z < v.position.z)
                higher_bounds_.z = v.position.z;

            if (lower_bounds_.x > v.position.x)
                lower_bounds_.x = v.position.x;
            if (lower_bounds_.y > v.position.y)
                lower_bounds_.y = v.position.y;
            if (lower_bounds_.z > v.position.z)
                lower_bounds_.z = v.position.z;
        }
        calculate_vertexs_();
    }
} // namespace raphEngine::objects
