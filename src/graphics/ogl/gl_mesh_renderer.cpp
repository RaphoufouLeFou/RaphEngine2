#include "graphics/ogl/gl_mesh_renderer.hpp"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>

#include "graphics/mesh_renderer.hpp"
#include "objects/mesh.hpp"
#include "settings/graphics.hpp"

namespace raphEngine::graphics
{

    GLMeshRenderer::GLMeshRenderer()
    {
    }

    void GLMeshRenderer::render(raphEngine::objects::Mesh* mesh)
    {
        // TODO: implement
        (void) mesh;

        // shaderUse->setMat4("model", ObjectModel /* * mesh->ModelMatrix*/);

        std::cout << "rendering using openGL\n";
        // glBindVertexArray(vao_);
    }

    void GLMeshRenderer::render_shadows(raphEngine::objects::Mesh* mesh)
    {
        // TODO: implement
        (void) mesh;
    }

} // namespace raphEngine::graphics
