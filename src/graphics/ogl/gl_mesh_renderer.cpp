#include "graphics/ogl/gl_mesh_renderer.hpp"
#include <glm/ext/vector_float3.hpp>
#include <numbers>
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
            sh->setValue("lightIntensity",
                         std::numbers::pi_v<float> * dir_light->intensity_);
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

        sh->setValue("texture_diffuse", 0);
        sh->setValue("texture_normal", 1);
        sh->setValue("texture_metallic", 2);
        sh->setValue("texture_roughness", 3);
        sh->setValue("texture_ao", 4);
        sh->setValue("texture_emissive", 5);
        sh->setValue("texture_opacity", 6);
        sh->setValue("texture_height", 7);

        glActiveTexture(GL_TEXTURE8);
        sh->setValue("shadowMap", 8);
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
            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->get_irradiance_map());
            sh->setValue("irradianceMap", 9);

            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->get_prefilter_map());
            sh->setValue("prefilterMap", 10);
            sh->setValue("maxPrefilterLod", ogl::GL_Skybox::kMaxPrefilterLod);

            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, skybox->get_brdf_lut());
            sh->setValue("brdfLUT", 11);

            sh->setValue("ambientIntensity", skybox->get_ambient_intensity());
            sh->setValue("reflectionExposure",
                         skybox->get_reflection_exposure());
        }
    }

    void BindMeshMaterial(const GlShader* sh, const objects::Mesh* mesh)
    {
        bool haveDiffuse = false, haveNormal = false, haveMetallic = false,
             haveRoughness = false, haveAO = false, haveEmissive = false,
             haveOpacity = false, haveHeight = false;

        for (const auto& tex : mesh->get_textures())
        {
            GLenum unit = 0;
            bool* flag = nullptr;

            switch (tex.type)
            {
            case objects::Texture::DIFFUSE:
                unit = GL_TEXTURE0;
                flag = &haveDiffuse;
                break;
            case objects::Texture::NORMAL:
                unit = GL_TEXTURE1;
                flag = &haveNormal;
                break;
            case objects::Texture::METALLIC:
                unit = GL_TEXTURE2;
                flag = &haveMetallic;
                break;
            case objects::Texture::ROUGHNESS:
                unit = GL_TEXTURE3;
                flag = &haveRoughness;
                break;
            case objects::Texture::AO:
                unit = GL_TEXTURE4;
                flag = &haveAO;
                break;
            case objects::Texture::EMISSIVE:
                unit = GL_TEXTURE5;
                flag = &haveEmissive;
                break;
            case objects::Texture::OPACITY:
                unit = GL_TEXTURE6;
                flag = &haveOpacity;
                break;
            case objects::Texture::HEIGHT:
                unit = GL_TEXTURE7;
                flag = &haveHeight;
                break;
            case objects::Texture::SPECULAR:
            default:
                continue;
            }

            glActiveTexture(unit);
            glBindTexture(GL_TEXTURE_2D, tex.id);
            *flag = true;
        }

        sh->setValue("HaveTexture", haveDiffuse);
        sh->setValue("HaveNormalMap", haveNormal);
        sh->setValue("HaveMetallicMap", haveMetallic);
        sh->setValue("HaveRoughnessMap", haveRoughness);
        sh->setValue("HaveAOMap", haveAO);
        sh->setValue("HaveEmissiveMap", haveEmissive);
        sh->setValue("HaveOpacityMap", haveOpacity);
        sh->setValue("HaveHeightMap", haveHeight);

        const resources::SubmeshData* data = mesh->data_;
        sh->setValue("metallicFactor", data->metallic_factor);
        sh->setValue("roughnessFactor", data->roughness_factor);
        sh->setValue("emissiveFactor", data->emissive_factor);
        sh->setValue("metallicRoughnessPacked",
                     data->metallic_roughness_packed);
        sh->setValue("alphaMask", data->alpha_mask);
        sh->setValue("alphaCutoff", data->alpha_cutoff);
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

        BindMeshMaterial(mesh_shader, mesh);

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

        BindMeshMaterial(mesh_shader, first); // batch key guarantees every
                                              // instance shares one material

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
