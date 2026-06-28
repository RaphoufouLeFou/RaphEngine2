#include "graphics/ogl/gl_shadow_renderer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
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
#include <RaphEngine2/inputs/keyboard.hpp>

namespace raphEngine::graphics
{
    GLShadowRenderer::GLShadowRenderer()
    {}

    unsigned int matricesUBO;
    unsigned int depthMapFBO;
    unsigned int GLShadowRenderer::depthMap;

    std::vector<GLuint> visualizerVAOs;
    std::vector<GLuint> visualizerVBOs;
    std::vector<GLuint> visualizerEBOs;
    std::vector<glm::mat4> lightMatricesCache;

    void GLShadowRenderer::drawCascadeVolumeVisualizers(
        const std::vector<glm::mat4>& lightMatrices, Shader* shader)
    {
        Logger::LogDebug("Debug drawing...");
        visualizerVAOs.resize(8);
        visualizerEBOs.resize(8);
        visualizerVBOs.resize(8);

        const GLuint indices[] = { 0, 2, 3, 0, 3, 1, 4, 6, 2, 4, 2, 0,
                                   5, 7, 6, 5, 6, 4, 1, 3, 7, 1, 7, 5,
                                   6, 7, 3, 6, 3, 2, 1, 5, 4, 0, 1, 4 };

        const glm::vec4 colors[] = {
            { 1.0, 0.0, 0.0, 0.5f },
            { 0.0, 1.0, 0.0, 0.5f },
            { 0.0, 0.0, 1.0, 0.5f },
        };

        for (size_t i = 0; i < lightMatrices.size(); ++i)
        {
            const auto corners = getFrustumCornersWorldSpace(lightMatrices[i]);
            std::vector<glm::vec3> vec3s;
            for (const auto& v : corners)
            {
                vec3s.push_back(glm::vec3(v));
            }

            glGenVertexArrays(1, &visualizerVAOs[i]);
            glGenBuffers(1, &visualizerVBOs[i]);
            glGenBuffers(1, &visualizerEBOs[i]);

            glBindVertexArray(visualizerVAOs[i]);

            glBindBuffer(GL_ARRAY_BUFFER, visualizerVBOs[i]);
            glBufferData(GL_ARRAY_BUFFER, vec3s.size() * sizeof(glm::vec3),
                         &vec3s[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, visualizerEBOs[i]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(GLuint),
                         &indices[0], GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                                  (void*)0);

            glBindVertexArray(visualizerVAOs[i]);
            shader->setValue("color", colors[i % 3]);
            glDrawElements(GL_TRIANGLES, GLsizei(36), GL_UNSIGNED_INT, 0);

            glDeleteBuffers(1, &visualizerVBOs[i]);
            glDeleteBuffers(1, &visualizerEBOs[i]);
            glDeleteVertexArrays(1, &visualizerVAOs[i]);

            glBindVertexArray(0);
        }

        visualizerVAOs.clear();
        visualizerEBOs.clear();
        visualizerVBOs.clear();
    }

    void GLShadowRenderer::generate_shadows_buffer()
    {
        glGenFramebuffers(1, &depthMapFBO);

        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthMap);

        int res = Settings::Get<GraphicsSettings>().getShadowResolution();
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, res, res,
                     int(shadowCascadeLevels.size()) + 1, 0, GL_DEPTH_COMPONENT,
                     GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_BORDER);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE,
                        GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC,
                        GL_LEQUAL);

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
        if (!shadow_shader)
        {
            shadow_shader = Shader::loadShader(default_shadow_vs_shader,
                                               default_shadow_fs_shader,
                                               default_shadow_gs_shader);
        }

        shadow_shader->use();

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.5f, 10.0f);
    }

    std::shared_ptr<Shader> debugCascadeShader = nullptr;

    void GLShadowRenderer::debug_draw_lights()
    {
        if (!debugCascadeShader)
        {
            debugCascadeShader = Shader::loadShader(debug_cascade_vs_shader,
                                                    debug_cascade_fs_shader);
        }

        static bool pressed = false;

        if (inputs::Key::IsKeyPressed(inputs::Key::KeyCode::KEY_E)
            && pressed == false)
        {
            Logger::LogInfo("E is pressed!");
            lightMatricesCache = getLightSpaceMatrices();
            pressed = true;
        }
        else if (!inputs::Key::IsKeyPressed(inputs::Key::KeyCode::KEY_E))
        {
            lightMatricesCache.clear();
            pressed = false;
        }

        component::CameraComponent* cam =
            component::CameraComponent::active_camera;

        if (lightMatricesCache.size() != 0)
        {
            Logger::LogDebug("Drawing lights mat");
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            debugCascadeShader->use();
            debugCascadeShader->setValue("projection", cam->projection_matrix_);
            debugCascadeShader->setValue("view", cam->view_matrix_);
            drawCascadeVolumeVisualizers(lightMatricesCache,
                                         debugCascadeShader.get());
            glDisable(GL_BLEND);
        }
    }

    void GLShadowRenderer::cleanup_shadows()
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
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

        if (!*mesh->cast_shadows)
        {
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
