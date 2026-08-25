#include <RaphEngine2/graphics/ogl/gl_outline_renderer.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <string>

#include "component/camera_component.hpp"
#include "graphics/graphic_api.hpp"
#include "graphics/ogl/gl_shader.hpp"
#include "graphics/ogl/gl_mesh_buffers.hpp"
#include "graphics/shader.hpp"
#include "objects/mesh.hpp"
#include "logger/logger.hpp"
#include "default_shaders.hpp"

namespace raphEngine::graphics::ogl
{
    void GL_OutlineRenderer::ensureResources(int width, int height)
    {
        if (dummy_vao_ == 0)
            glGenVertexArrays(1, &dummy_vao_);

        if (mask_fbo_ != 0 && width == fbo_width_ && height == fbo_height_)
            return;

        if (mask_fbo_ != 0)
        {
            glDeleteFramebuffers(1, &mask_fbo_);
            glDeleteTextures(1, &mask_tex_);
            glDeleteTextures(1, &mask_depth_tex_);
            glDeleteFramebuffers(1, &intermediate_fbo_);
            glDeleteTextures(1, &intermediate_tex_);
        }

        fbo_width_ = width;
        fbo_height_ = height;

        glGenFramebuffers(1, &mask_fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, mask_fbo_);

        glGenTextures(1, &mask_tex_);
        glBindTexture(GL_TEXTURE_2D, mask_tex_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, mask_tex_, 0);

        glGenTextures(1, &mask_depth_tex_);
        glBindTexture(GL_TEXTURE_2D, mask_depth_tex_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0,
                     GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                               GL_TEXTURE_2D, mask_depth_tex_, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            Logger::LogError("Outline mask framebuffer is not complete!");

        glGenFramebuffers(1, &intermediate_fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, intermediate_fbo_);

        glGenTextures(1, &intermediate_tex_);
        glBindTexture(GL_TEXTURE_2D, intermediate_tex_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG,
                     GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, intermediate_tex_, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            Logger::LogError(
                "Outline intermediate framebuffer is not complete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GL_OutlineRenderer::render(
        const std::vector<const objects::Mesh*>& outlined_meshes)
    {
        if (outlined_meshes.empty())
            return;

        component::CameraComponent* cam =
            component::CameraComponent::active_camera;
        if (!cam)
            return;

        int width = graphics::GraphicApi::res_x;
        int height = graphics::GraphicApi::res_y;
        if (width <= 0 || height <= 0)
            return;

        ensureResources(width, height);

        if (!mask_shader_)
            mask_shader_ = Shader::loadShader(outline_mask_vs_shader,
                                              outline_mask_fs_shader);
        if (!dilate_h_shader_)
            dilate_h_shader_ = Shader::loadShader(fullscreen_triangle_vs_shader,
                                                  outline_dilate_h_fs_shader);
        if (!dilate_v_composite_shader_)
            dilate_v_composite_shader_ =
                Shader::loadShader(fullscreen_triangle_vs_shader,
                                   outline_dilate_v_composite_fs_shader);

        GLint scene_fbo = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &scene_fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, scene_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mask_fbo_);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, mask_fbo_);
        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-2.0f, -2.0f);

        const GlShader* mask_sh =
            dynamic_cast<const GlShader*>(mask_shader_.get());
        mask_sh->use();
        mask_sh->setValue("view", cam->view_matrix_);
        mask_sh->setValue("projection", cam->projection_matrix_);

        for (const objects::Mesh* mesh : outlined_meshes)
        {
            const auto* buffers = dynamic_cast<const graphics::GLMeshBuffers*>(
                mesh->get_buffers());
            if (!buffers)
                continue;

            glm::mat4 model =
                mesh->parent_object->get_transform().get_model_matrix()
                * mesh->get_model_matrix();
            mask_sh->setValue("model", model);

            glBindVertexArray(buffers->vao_);
            glDrawElements(
                GL_TRIANGLES,
                static_cast<unsigned int>(mesh->get_indices().size()),
                GL_UNSIGNED_INT, 0);
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_BLEND);

        // --- Horizontal dilate: propagate coverage + nearest trustworthy
        // depth together ---
        glBindFramebuffer(GL_FRAMEBUFFER, intermediate_fbo_);
        glViewport(0, 0, width, height);
        glDisable(GL_BLEND);
        glBindVertexArray(dummy_vao_);

        const GlShader* dh =
            dynamic_cast<const GlShader*>(dilate_h_shader_.get());
        dh->use();
        dh->setValue("radius", outline_width_px_);
        dh->setValue("texelSize", glm::vec2(1.0f / width, 1.0f / height));
        dh->setValue("maskTex", 0);
        dh->setValue("depthTex", 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mask_tex_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mask_depth_tex_);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // --- Vertical dilate + occlusion-aware composite onto the scene ---
        glBindFramebuffer(GL_FRAMEBUFFER, scene_fbo);
        glViewport(0, 0, width, height);
        glEnable(GL_BLEND);

        const GlShader* dvc =
            dynamic_cast<const GlShader*>(dilate_v_composite_shader_.get());
        dvc->use();
        dvc->setValue("radius", outline_width_px_);
        dvc->setValue("texelSize", glm::vec2(1.0f / width, 1.0f / height));
        dvc->setValue("outlineColor", outline_color_);
        dvc->setValue("hDilatedTex", 0);
        dvc->setValue("originalMaskTex", 1);
        dvc->setValue("sceneDepthTex", 2);
        dvc->setValue("nearPlane", cam->nearPlane);
        dvc->setValue("farPlane", cam->farPlane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, intermediate_tex_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mask_tex_);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mask_depth_tex_);

        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }
} // namespace raphEngine::graphics::ogl
