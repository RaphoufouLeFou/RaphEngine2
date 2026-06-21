#include "graphics/ogl/gl_mesh_renderer.hpp"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>

#include "objects/mesh.hpp"
#include "graphics/mesh_renderer.hpp"
#include "settings/graphics.hpp"
#include "RaphEngine2/component/camera_component.hpp"

namespace raphEngine::graphics
{

    const GlShader* GLMeshRenderer::current_active_shader_ = nullptr;

    GLMeshRenderer::GLMeshRenderer()
    {

    }

    void SetupShader(const GlShader * sh)
    {

        component::CameraComponent* cam = component::CameraComponent::active_camera;

        sh->setValue("projection", cam->projection_matrix_);
        sh->setValue("view", cam->view_matrix_);

        sh->setValue("lightPos", glm::vec3(0));
        sh->setValue("lightDir", glm::vec3(0));
        sh->setValue("viewPos", cam->parent_object->get_transform().get_position());
        sh->setValue("farPlane", cam->farPlane);

        // sh->setValue("lightSpaceMatrix", lightSpaceMatrix);
        // sh->setInt("cascadeCount", shadowCascadeLevels.size());
/*
        for (size_t i = 0; i < shadowCascadeLevels.size(); i++)
        {
            sh->setFloat(("cascadePlaneDistances[" + std::to_string(i) + "]").c_str(), shadowCascadeLevels[i]);
        }

        for(int i = 0; i < SAMPLE_SIZE; i++)
        {
            std::string name = "offsets[" + std::to_string(i) + "]";
            glUniform2f(glGetUniformLocation(sh->ID, name.c_str()), offsets[i].x, offsets[i].y);
        }

        //sh->setVec2Array("offsets", 64, offsets);
*/
        const char* names[] = { "texture_diffuse", "texture_specular", "texture_normal", "texture_height", "shadowMap" };
        for (int i = 0; i < 5; i++)
        {
            sh->setValue(names[i], i);
        }

/*
        glActiveTexture(GL_TEXTURE4);
        sh->setValue("shadowMap", 4);
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthMap);
*/
    }

    void GLMeshRenderer::render(const raphEngine::objects::Mesh* mesh) const
    {
        if (!component::CameraComponent::active_camera)
        {
            std::cerr << "Cant render a mesh with no active camera!" << std::endl;
            return;
        }

        const Shader* s = mesh->get_shader();

        if (!s)
        {
            std::cerr << "Cant render a mesh with no shader!" << std::endl;
            return;
        }

        if(mesh->get_vertices().size() == 0)
        {
            std::cerr << "Cant render a mesh with no vertices!" << std::endl;
            return;
        }

        const GlShader* mesh_shader = dynamic_cast<const GlShader*>(s);
        
        if (mesh_shader != current_active_shader_)
        {
            current_active_shader_ = mesh_shader;
            mesh_shader->use();
            SetupShader(mesh_shader);
        }

        component::CameraComponent::active_camera->calculate_matrices();


        
        mesh_shader->setValue("model", mesh->parent_object->get_transform().get_model_matrix()/* * mesh->ModelMatrix*/);

        std::cout << "rendering using openGL\n";
        // glBindVertexArray(vao_);
    }

    void GLMeshRenderer::render_shadows(const raphEngine::objects::Mesh* mesh) const
    {
        // TODO: implement
        (void) mesh;
    }

} // namespace raphEngine::graphics
