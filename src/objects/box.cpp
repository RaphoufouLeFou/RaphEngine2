#include "RaphEngine2/objects/box.hpp"
#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>

namespace raphEngine::objects
{
    void Box::render()
    {
        
    }

    void Box::calculate_vertexs()
    {
        vertices_.reserve(8);
        indices_.reserve(12*3);

        /*
        
              2--------3
             /|       /|
            6--------7 |
            | |      | |
            | 0------|-1
            |/       |/
            4--------5
        
        */

        vertices_.push_back(Vertex{glm::vec3{lower_bounds_.x, lower_bounds_.y, lower_bounds_.z}});
        vertices_.push_back(Vertex{glm::vec3{higher_bounds_.x, lower_bounds_.y, lower_bounds_.z}});
        vertices_.push_back(Vertex{glm::vec3{lower_bounds_.x, higher_bounds_.y, lower_bounds_.z}});
        vertices_.push_back(Vertex{glm::vec3{higher_bounds_.x, higher_bounds_.y, lower_bounds_.z}});

        vertices_.push_back(Vertex{glm::vec3{lower_bounds_.x, lower_bounds_.y, higher_bounds_.z}});
        vertices_.push_back(Vertex{glm::vec3{higher_bounds_.x, lower_bounds_.y, higher_bounds_.z}});
        vertices_.push_back(Vertex{glm::vec3{lower_bounds_.x, higher_bounds_.y, higher_bounds_.z}});
        vertices_.push_back(Vertex{glm::vec3{higher_bounds_.x, higher_bounds_.y, higher_bounds_.z}});

        constexpr int triangles = 0;
    }

    void Box::create_bounding_box(const Mesh& mesh)
    {
        if (mesh.get_vertices().size() == 0)
        {
            lower_bounds_ = glm::vec3{0};
            higher_bounds_ = glm::vec3{0};
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
        calculate_vertexs();
    }
}