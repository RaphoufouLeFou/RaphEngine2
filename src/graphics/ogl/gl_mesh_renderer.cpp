#include "graphics/ogl/gl_mesh_renderer.hpp"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <memory>

#include "RaphEngine2/component/camera_component.hpp"
#include "graphics/mesh_renderer.hpp"
#include "graphics/ogl/gl_mesh_buffers.hpp"
#include "graphics/ogl/gl_shader.hpp"
#include "graphics/graphic_api.hpp"
#include "graphics/shader.hpp"
#include "objects/mesh.hpp"
#include "settings/graphics.hpp"
#include <RaphEngine2/settings/settings.hpp>
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/graphics/shadow_renderer.hpp>

namespace raphEngine::graphics
{

    const GlShader* GLMeshRenderer::current_active_shader_ = nullptr;

    GLMeshRenderer::GLMeshRenderer()
    {}

    void SetupShader(const GlShader* sh)
    {
        component::CameraComponent* cam =
            component::CameraComponent::active_camera;

        sh->setValue("projection", cam->projection_matrix_);
        sh->setValue("view", cam->view_matrix_);

        sh->setValue("lightPos", glm::vec3(0));
        sh->setValue("lightDir", ShadowRenderer::lightDirGlobal);

        sh->setValue("viewPos",
                     cam->parent_object->get_transform().get_position());
        sh->setValue("farPlane", cam->farPlane);

        // sh->setValue("lightSpaceMatrix", lightSpaceMatrix);
        // sh->setInt("cascadeCount", shadowCascadeLevels.size());
        /*
                for (size_t i = 0; i < shadowCascadeLevels.size(); i++)
                {
                    sh->setFloat(("cascadePlaneDistances[" + std::to_string(i) +
           "]").c_str(), shadowCascadeLevels[i]);
                }

                for(int i = 0; i < SAMPLE_SIZE; i++)
                {
                    std::string name = "offsets[" + std::to_string(i) + "]";
                    glUniform2f(glGetUniformLocation(sh->ID, name.c_str()),
           offsets[i].x, offsets[i].y);
                }

                //sh->setVec2Array("offsets", 64, offsets);
        */

        // TODO: rework this, there for sure is a better way to to it
        const char* names[] = { "texture_diffuse", "texture_normal",
                                "texture_specular", "texture_height",
                                "shadowMap" };
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
            Logger::LogError("Cant render a mesh with no active camera!");
            return;
        }

        const Shader* s = mesh->get_shader();

        if (!s)
        {
            Logger::LogError("Cant render a mesh with no shader!");
            return;
        }

        if (mesh->get_vertices().size() == 0)
        {
            Logger::LogError("Cant render a mesh with no vertices!");
            return;
        }

        const GlShader* mesh_shader = dynamic_cast<const GlShader*>(s);

        component::CameraComponent::active_camera->calculate_matrices();

        if (mesh_shader != current_active_shader_)
        {
            current_active_shader_ = mesh_shader;
            mesh_shader->use();
        }

        SetupShader(mesh_shader);

        mesh_shader->setValue(
            "model",
            mesh->model_matrix_
                * mesh->parent_object->get_transform()
                      .get_model_matrix() /* * mesh->ModelMatrix*/);

        bool HaveTexture = false;
        bool HaveNormalMap = false;
        bool HaveSpecularMap = false;
        bool HaveHeightMap = false;

        for (size_t i = 0; i < mesh->get_textures().size(); i++)
        {
            if (mesh->get_textures().at(i).type == objects::Texture::DIFFUSE)
                HaveTexture = true;
            if (mesh->get_textures().at(i).type == objects::Texture::NORMAL)
                HaveNormalMap = true;
            if (mesh->get_textures().at(i).type == objects::Texture::SPECULAR)
                HaveSpecularMap = true;
            if (mesh->get_textures().at(i).type == objects::Texture::HEIGHT)
                HaveHeightMap = true;

            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, mesh->get_textures().at(i).id);
        }

        mesh_shader->setValue("HaveTexture", HaveTexture);
        mesh_shader->setValue("HaveNormalMap", HaveNormalMap);
        mesh_shader->setValue("HaveSpecularMap", HaveSpecularMap);
        mesh_shader->setValue("HaveHeightMap", HaveHeightMap);

        const graphics::GLMeshBuffers* mesh_buffers =
            dynamic_cast<const graphics::GLMeshBuffers*>(mesh->get_buffers());

        glBindVertexArray(mesh_buffers->vao_);

        // glm::vec3 lower_bounds = glm::vec3(mesh->model_matrix_ *
        // mesh->parent_object->get_transform().get_model_matrix() *
        // glm::vec4(mesh->get_lower_bounds(), 1.0)); glm::vec3 higher_bounds =
        // glm::vec3(mesh->model_matrix_ *
        // mesh->parent_object->get_transform().get_model_matrix() *
        // glm::vec4(mesh->get_higher_bounds(), 1.0));

        /*
        printf("rendering mesh withs mins of %.2f, %.2f, %.2f, and max of %.2f,
        %.2f, %.2f\n", lower_bounds.x, lower_bounds.y, lower_bounds.z,
            higher_bounds.x, higher_bounds.y, higher_bounds.z
        );
        */

        glDrawElements(GL_TRIANGLES,
                       static_cast<unsigned int>(mesh->get_indices().size()),
                       GL_UNSIGNED_INT, 0);
    }
} // namespace raphEngine::graphics
