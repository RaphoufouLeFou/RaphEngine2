#include "graphics/ogl/gl_mesh_buffers.hpp"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <iostream>

#include "graphics/mesh_buffers.hpp"
#include "objects/mesh.hpp"

namespace raphEngine::graphics
{

    GLMeshBuffers::GLMeshBuffers(raphEngine::objects::Mesh* mesh)
    {
        GenerateBuffers(mesh);
    }

    void GLMeshBuffers::GenerateBuffers(raphEngine::objects::Mesh* mesh)
    {
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        glBufferData(GL_ARRAY_BUFFER,
                     mesh->get_vertices().size() * sizeof(objects::Vertex),
                     &mesh->get_vertices()[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     mesh->get_indices().size() * sizeof(unsigned int),
                     &mesh->get_indices()[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(objects::Vertex),
                              static_cast<void*>(0));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(objects::Vertex),
                              (void*)offsetof(objects::Vertex, normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(objects::Vertex),
                              (void*)offsetof(objects::Vertex, tex_coords));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(objects::Vertex),
                              (void*)offsetof(objects::Vertex, tangent));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(objects::Vertex),
                              (void*)offsetof(objects::Vertex, bitangent));
    }
    
} // namespace raphEngine::graphics
