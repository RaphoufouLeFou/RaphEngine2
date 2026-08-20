#include <RaphEngine2/graphics/ogl/gl_skybox.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "default_shaders.hpp"
#include <memory>
#include <string>

#include "component/camera_component.hpp"
#include "graphics/graphic_api.hpp"
#include "graphics/ogl/gl_shader.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture_loader.hpp"
#include "logger/logger.hpp"

namespace raphEngine::graphics::ogl
{
    namespace
    {
        constexpr unsigned int kCaptureFaceSize = 1024;

        constexpr float kCubeVertices[] = {
            -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
            1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

            -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
            -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

            1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

            -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
            1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f
        };

        void CreateCubeGeometry(unsigned int& vao, unsigned int& vbo)
        {
            if (vao != 0)
                return;

            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices,
                         GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                                  (void*)0);
            glBindVertexArray(0);
        }
    } // namespace

    void GL_Skybox::set_hdr(const std::string& hdr)
    {
        auto raw = TextureLoader::getInstance()->load_texture_raw_hdr(hdr);
        if (!raw.data)
        {
            Logger::LogError("Failed to load HDR skybox: ", hdr);
            return;
        }

        GLenum internal_fmt = GL_RGB16F, fmt = GL_RGB;
        if (raw.nrChannels == 1)
        {
            internal_fmt = GL_R16F;
            fmt = GL_RED;
        }
        else if (raw.nrChannels == 4)
        {
            internal_fmt = GL_RGBA16F;
            fmt = GL_RGBA;
        }

        unsigned int hdr_texture;
        glGenTextures(1, &hdr_texture);
        glBindTexture(GL_TEXTURE_2D, hdr_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internal_fmt, raw.width, raw.height, 0,
                     fmt, GL_FLOAT, raw.data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        TextureLoader::getInstance()->free_raw(raw);

        if (cube_map_buffer_ != 0)
            glDeleteTextures(1, &cube_map_buffer_);

        glGenTextures(1, &cube_map_buffer_);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_buffer_);
        for (unsigned int i = 0; i < 6; i++)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                         kCaptureFaceSize, kCaptureFaceSize, 0, GL_RGB,
                         GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        CreateCubeGeometry(skybox_vao_, skybox_vbo_);

        unsigned int capture_fbo, capture_rbo;
        glGenFramebuffers(1, &capture_fbo);
        glGenRenderbuffers(1, &capture_rbo);
        glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                              kCaptureFaceSize, kCaptureFaceSize);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, capture_rbo);

        auto conv_shader = Shader::loadShader(equirect_to_cubemap_vs_shader,
                                              equirect_to_cubemap_fs_shader);
        const GlShader* conv = dynamic_cast<const GlShader*>(conv_shader.get());

        const glm::mat4 capture_projection =
            glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

        const glm::mat4 capture_views[6] = {
            glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)),
        };

        conv->use();
        conv->setValue("equirectangularMap", 0);
        conv->setValue("projection", capture_projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdr_texture);

        glDisable(GL_CULL_FACE);
        glViewport(0, 0, kCaptureFaceSize, kCaptureFaceSize);
        glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);

        for (unsigned int i = 0; i < 6; i++)
        {
            conv->setValue("view", capture_views[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   cube_map_buffer_, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindVertexArray(skybox_vao_);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_CULL_FACE);
        glViewport(0, 0, GraphicApi::res_x, GraphicApi::res_y);

        glDeleteFramebuffers(1, &capture_fbo);
        glDeleteRenderbuffers(1, &capture_rbo);
        glDeleteTextures(1, &hdr_texture);

        if (!skybox_shader_)
            skybox_shader_ =
                Shader::loadShader(skybox_vs_shader, skybox_fs_shader);
    }

    void GL_Skybox::render()
    {
        if (cube_map_buffer_ == 0 || !skybox_shader_)
            return;

        component::CameraComponent* cam =
            component::CameraComponent::active_camera;
        if (!cam)
            return;

        const GlShader* sh =
            dynamic_cast<const GlShader*>(skybox_shader_.get());

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        sh->use();
        sh->setValue("view", glm::mat4(glm::mat3(cam->view_matrix_)));
        sh->setValue("projection", cam->projection_matrix_);
        sh->setValue("skybox", 0);
        sh->setValue("exposure", exposure_);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_buffer_);

        glBindVertexArray(skybox_vao_);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }
} // namespace raphEngine::graphics::ogl
