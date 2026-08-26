#include "graphics/ogl/gl_mesh_renderer.hpp"
#include <glm/ext/vector_float3.hpp>
#include <string>
#include "graphics/ogl/gl_skybox.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <memory>

#include "graphics/ogl/gl_shadow_renderer.hpp"
#include "RaphEngine2/component/camera_component.hpp"
#include "graphics/mesh_renderer.hpp"
#include "graphics/ogl/gl_mesh_buffers.hpp"
#include "graphics/ogl/gl_shader.hpp"
#include "graphics/shader.hpp"
#include "objects/mesh.hpp"
#include "utils.hpp"
#include <RaphEngine2/settings/settings.hpp>
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/graphics/shadow_renderer.hpp>
#include <RaphEngine2/default_shaders.hpp>
#include <RaphEngine2/resources/model_resource.hpp>

namespace raphEngine::graphics
{

    const GlShader* GLMeshRenderer::current_active_shader_ = nullptr;

    std::shared_ptr<Shader> default_mesh_shader_instanced = nullptr;

    GLMeshRenderer::GLMeshRenderer()
    {}

    void SetupShader(const GlShader* sh)
    {
        Camera* cam = Camera::get_active_camera();

        sh->setValue("projection", cam->get_projection_matrix_());
        sh->setValue("view", cam->get_view_matrix_());

        const auto* dir_light = ShadowRenderer::GetDirectionalLight();

        if (dir_light)
        {
            sh->setValue("lightIntensity", dir_light->intensity_);
            sh->setValue("lightDir",
                         Utils::GetForwardFromModelMatrix(
                             dir_light->parent_object->get_transform()
                                 .get_model_matrix()));
        }
        else
        {
            sh->setValue("lightIntensity", 0.0f);
            sh->setValue("lightDir", glm::vec3(0));
        }

        sh->setValue("viewPos", cam->get_position());
        sh->setValue("farPlane", cam->get_farPlane());

        sh->setValue("cascadeCount",
                     (int)ShadowRenderer::shadowCascadeLevels.size());

        for (size_t i = 0; i < ShadowRenderer::shadowCascadeLevels.size(); i++)
        {
            sh->setValue(
                ("cascadePlaneDistances[" + std::to_string(i) + "]").c_str(),
                cam->get_farPlane() / ShadowRenderer::shadowCascadeLevels[i]);
        }

        const char* names[] = { "texture_diffuse", "texture_normal",
                                "texture_specular", "texture_height" };
        for (int i = 0; i < 4; i++)
        {
            sh->setValue(names[i], i);
        }

        glActiveTexture(GL_TEXTURE4);
        sh->setValue("shadowMap", 4);
        glBindTexture(
            GL_TEXTURE_2D_ARRAY,
            dynamic_cast<GLShadowRenderer*>(ShadowRenderer::getInstance())
                ->depthMap);

        ogl::GL_Skybox* skybox =
            dynamic_cast<ogl::GL_Skybox*>(Skybox::getInstance());
        bool have_skybox = skybox && skybox->is_loaded();
        sh->setValue("haveSkybox", have_skybox);

        if (have_skybox)
        {
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->get_irradiance_map());
            sh->setValue("irradianceMap", 5);

            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->get_environment_map());
            sh->setValue("environmentMap", 6);
            sh->setValue("maxReflectionLod", ogl::GL_Skybox::kMaxReflectionLod);
            sh->setValue("ambientIntensity", skybox->get_ambient_intensity());
            sh->setValue("reflectionExposure",
                         skybox->get_reflection_exposure());
        }
    }

    void GLMeshRenderer::render(const raphEngine::objects::Mesh* mesh) const
    {
        if (!Camera::get_active_camera())
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

        Camera::get_active_camera()->calculate_matrices();

        if (mesh_shader != current_active_shader_)
        {
            current_active_shader_ = mesh_shader;
            mesh_shader->use();
        }

        SetupShader(mesh_shader);

        mesh_shader->setValue(
            "model",
            mesh->parent_object->get_transform().get_model_matrix()
                * mesh->get_model_matrix());

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

        glDrawElements(GL_TRIANGLES,
                       static_cast<unsigned int>(mesh->get_indices().size()),
                       GL_UNSIGNED_INT, 0);
    }

    void GLMeshRenderer::renderInstanced(
        const std::vector<const objects::Mesh*>& meshes) const
    {
        if (!Camera::get_active_camera())
        {
            Logger::LogError("Cant render a mesh with no active camera!");
            return;
        }

        if (meshes.empty())
            return;

        const objects::Mesh* first = meshes.front();

        if (first->get_vertices().size() == 0)
        {
            Logger::LogError("Cant render a mesh with no vertices!");
            return;
        }

        if (!default_mesh_shader_instanced)
        {
            default_mesh_shader_instanced = Shader::loadShader(
                default_instanced_vs_shader, default_fs_shader);
        }

        const GlShader* mesh_shader =
            dynamic_cast<const GlShader*>(default_mesh_shader_instanced.get());

        Camera::get_active_camera()->calculate_matrices();

        if (mesh_shader != current_active_shader_)
        {
            current_active_shader_ = mesh_shader;
            mesh_shader->use();
        }

        SetupShader(mesh_shader); // once per batch, not per mesh

        bool HaveTexture = false;
        bool HaveNormalMap = false;
        bool HaveSpecularMap = false;
        bool HaveHeightMap = false;

        for (size_t i = 0; i < first->get_textures().size(); i++)
        {
            if (first->get_textures().at(i).type == objects::Texture::DIFFUSE)
                HaveTexture = true;
            if (first->get_textures().at(i).type == objects::Texture::NORMAL)
                HaveNormalMap = true;
            if (first->get_textures().at(i).type == objects::Texture::SPECULAR)
                HaveSpecularMap = true;
            if (first->get_textures().at(i).type == objects::Texture::HEIGHT)
                HaveHeightMap = true;

            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, first->get_textures().at(i).id);
        }

        mesh_shader->setValue("HaveTexture", HaveTexture);
        mesh_shader->setValue("HaveNormalMap", HaveNormalMap);
        mesh_shader->setValue("HaveSpecularMap", HaveSpecularMap);
        mesh_shader->setValue("HaveHeightMap", HaveHeightMap);

        std::vector<glm::mat4> worlds;
        worlds.reserve(meshes.size());
        for (const objects::Mesh* m : meshes)
            worlds.push_back(
                m->parent_object->get_transform().get_model_matrix()
                * m->get_model_matrix());

        auto* buffers = const_cast<graphics::GLMeshBuffers*>(
            dynamic_cast<const graphics::GLMeshBuffers*>(first->get_buffers()));
        buffers->UploadInstanceData(worlds);
        /*
                Logger::LogDebug("Drawing ", std::to_string(worlds.size()),
                                 " meshes at the same time !");
        */
        glBindVertexArray(buffers->vao_);
        glDrawElementsInstanced(
            GL_TRIANGLES,
            static_cast<unsigned int>(first->get_indices().size()),
            GL_UNSIGNED_INT, 0, static_cast<GLsizei>(worlds.size()));
    }
} // namespace raphEngine::graphics
