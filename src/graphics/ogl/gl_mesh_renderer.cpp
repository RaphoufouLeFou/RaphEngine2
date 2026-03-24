#include <RaphEngine2/export.hpp>
#include <memory>
#include "graphics/mesh_renderer.hpp"
#include "graphics/ogl/gl_mesh_renderer.hpp"
#include "settings/graphics.hpp"
#include "objects/mesh.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <glm/glm.hpp>

namespace raphEngine::graphics {

    GLMeshRenderer::GLMeshRenderer(objects::Mesh * mesh)
        : MeshRenderer(mesh)
    {
        GenerateBuffers();
    }

    void GLMeshRenderer::render()
    {
        // TODO: implement


        // shaderUse->setMat4("model", ObjectModel /* * mesh->ModelMatrix*/);
        glBindVertexArray(vao_);

    }

    
    void GLMeshRenderer::render_shadows()
    {
        // TODO: implement
    }

    void GLMeshRenderer::GenerateBuffers()
    {
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);
    
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        glBufferData(GL_ARRAY_BUFFER, mesh_->get_vertices().size() * sizeof(objects::Vertex), &mesh_->get_vertices()[0], GL_STATIC_DRAW);
    
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_->get_indices().size() * sizeof(unsigned int), &mesh_->get_indices()[0], GL_STATIC_DRAW);
    
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(objects::Vertex), static_cast<void*> (0));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(objects::Vertex), (void*)offsetof(objects::Vertex, normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(objects::Vertex), (void*)offsetof(objects::Vertex, tex_coords));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(objects::Vertex), (void*)offsetof(objects::Vertex, tangent));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(objects::Vertex), (void*)offsetof(objects::Vertex, bitangent));
    }
}
