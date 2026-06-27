#include "graphics/ogl/gl_shadow_renderer.hpp"

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
#include <RaphEngine2/default_shaders.hpp>

namespace raphEngine::graphics
{

    // TODO: Change it to the light component
    glm::vec3 lightDirGlobal = glm::normalize(glm::vec3(1, 1, 1));

    GLShadowRenderer::GLShadowRenderer()
    {}

    unsigned int matricesUBO;
    unsigned int depthMapFBO;
    unsigned int GLShadowRenderer::depthMap;

    void GLShadowRenderer::generate_shadows_buffer()
    {
        glGenFramebuffers(1, &depthMapFBO);

        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthMap);

        int res = Settings::Get<GraphicsSettings>().getShadowResolution();
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, res, res,
                     int(shadowCascadeLevels.size()) + 1, 0, GL_DEPTH_COMPONENT,
                     GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_BORDER);

        constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR,
                         bordercolor);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            Logger::LogWarning("Framebuffer is not complete! Status: ", status);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glGenBuffers(1, &matricesUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 16, nullptr,
                     GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void GLShadowRenderer::prepare_shadows()
    {
        // 0. UBO setup
        int res = Settings::Get<GraphicsSettings>().getShadowResolution();

        const auto lightMatrices = getLightSpaceMatrices();

        glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
        for (size_t i = 0; i < lightMatrices.size(); i++)
        {
            glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4),
                            sizeof(glm::mat4x4), &lightMatrices[i]);
        }
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glViewport(0, 0, res, res);
        glClear(GL_DEPTH_BUFFER_BIT);
        // glCullFace(GL_FRONT); // peter panning
        if (!shadow_shader)
        {
            shadow_shader = Shader::loadShader(default_shadow_vs_shader,
                                               default_shadow_fs_shader,
                                               default_shadow_gs_shader);
        }
        shadow_shader->use();
    }

    void GLShadowRenderer::cleanup_shadows()
    {
        // glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLShadowRenderer::render_shadows(
        const raphEngine::objects::Mesh* mesh) const
    {
        if (!shadow_shader)
        {
            Logger::LogError(
                "Cant render a mesh shadow with no shadow shader!");
            return;
        }

        if (mesh->get_vertices().size() == 0)
        {
            Logger::LogError("Cant render a mesh shadow with no vertices!");
            return;
        }

        shadow_shader->setValue(
            "model",
            mesh->model_matrix_
                * mesh->parent_object->get_transform().get_model_matrix());

        const graphics::GLMeshBuffers* mesh_buffers =
            dynamic_cast<const graphics::GLMeshBuffers*>(mesh->get_buffers());

        glBindVertexArray(mesh_buffers->vao_);

        glDrawElements(GL_TRIANGLES,
                       static_cast<unsigned int>(mesh->get_indices().size()),
                       GL_UNSIGNED_INT, 0);
    }

} // namespace raphEngine::graphics
