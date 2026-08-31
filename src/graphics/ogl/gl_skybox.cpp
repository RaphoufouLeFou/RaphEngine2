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
#include <cmath>

#include "graphics/camera.hpp"
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
        constexpr unsigned int kIrradianceFaceSize = 32;

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

        const glm::mat4 kCaptureProjection =
            glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        const glm::mat4 kCaptureViews[6] = {
            glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),
            glm::lookAt(glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)),
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

        constexpr unsigned int kBrdfLutSize = 512;

        void GeneratePrefilterMap(unsigned int environment_map,
                                  unsigned int& prefilter_map,
                                  unsigned int skybox_vao,
                                  unsigned int capture_fbo,
                                  unsigned int capture_rbo,
                                  float ibl_radiance_clamp)
        {
            if (prefilter_map != 0)
                glDeleteTextures(1, &prefilter_map);

            glGenTextures(1, &prefilter_map);
            glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_map);
            for (unsigned int i = 0; i < 6; i++)
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                             GL_Skybox::kPrefilterBaseSize,
                             GL_Skybox::kPrefilterBaseSize, 0, GL_RGB, GL_FLOAT,
                             nullptr);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                            GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);

            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL,
                            GL_Skybox::kPrefilterMipLevels - 1);

            auto prefilter_shader = Shader::loadShader(
                cubemap_capture_vs_shader, prefilter_convolution_fs_shader);
            const GlShader* pf =
                dynamic_cast<const GlShader*>(prefilter_shader.get());

            pf->use();
            pf->setValue("environmentMap", 0);
            pf->setValue("projection", kCaptureProjection);
            pf->setValue("envResolution", float(kCaptureFaceSize));
            pf->setValue("maxRadiance", ibl_radiance_clamp);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, environment_map);

            glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
            glBindVertexArray(skybox_vao);

            for (unsigned int mip = 0; mip < GL_Skybox::kPrefilterMipLevels;
                 mip++)
            {
                unsigned int mipSize = GL_Skybox::kPrefilterBaseSize >> mip;
                glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                                      mipSize, mipSize);
                glViewport(0, 0, mipSize, mipSize);

                float roughness =
                    float(mip) / float(GL_Skybox::kPrefilterMipLevels - 1);
                pf->setValue("roughness", roughness);

                for (unsigned int face = 0; face < 6; face++)
                {
                    pf->setValue("view", kCaptureViews[face]);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                           GL_TEXTURE_CUBE_MAP_POSITIVE_X
                                               + face,
                                           prefilter_map, mip);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }

            glBindVertexArray(0);
        }

        void GenerateBRDFLUT(unsigned int& brdf_lut)
        {
            if (brdf_lut != 0)
                return;

            glGenTextures(1, &brdf_lut);
            glBindTexture(GL_TEXTURE_2D, brdf_lut);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, kBrdfLutSize, kBrdfLutSize,
                         0, GL_RG, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            unsigned int fbo, rbo;
            glGenFramebuffers(1, &fbo);
            glGenRenderbuffers(1, &rbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                                  kBrdfLutSize, kBrdfLutSize);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER, rbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, brdf_lut, 0);

            int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
                Logger::LogWarning(
                    "BRDF LUT framebuffer is not complete! Status: ", status);

            glViewport(0, 0, kBrdfLutSize, kBrdfLutSize);

            auto brdf_shader = Shader::loadShader(fullscreen_triangle_vs_shader,
                                                  brdf_lut_fs_shader);
            brdf_shader->use();

            // fullscreen_triangle_vs_shader reads no vertex attributes, but
            // core profile still requires a bound VAO for glDrawArrays.
            unsigned int dummy_vao;
            glGenVertexArrays(1, &dummy_vao);
            glBindVertexArray(dummy_vao);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glBindVertexArray(0);
            glDeleteVertexArrays(1, &dummy_vao);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &fbo);
            glDeleteRenderbuffers(1, &rbo);
        }
    } // namespace

    void GL_Skybox::set_hdr(const fs::path& hdr)
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
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        if (irradiance_map_ != 0)
            glDeleteTextures(1, &irradiance_map_);

        glGenTextures(1, &irradiance_map_);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_map_);
        for (unsigned int i = 0; i < 6; i++)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                         kIrradianceFaceSize, kIrradianceFaceSize, 0, GL_RGB,
                         GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

        glDisable(GL_CULL_FACE);
        glBindVertexArray(skybox_vao_);

        {
            auto conv_shader = Shader::loadShader(
                cubemap_capture_vs_shader, equirect_to_cubemap_fs_shader);
            const GlShader* conv =
                dynamic_cast<const GlShader*>(conv_shader.get());

            conv->use();
            conv->setValue("equirectangularMap", 0);
            conv->setValue("projection", kCaptureProjection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, hdr_texture);

            glViewport(0, 0, kCaptureFaceSize, kCaptureFaceSize);
            glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);

            for (unsigned int i = 0; i < 6; i++)
            {
                conv->setValue("view", kCaptureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                       cube_map_buffer_, 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_buffer_);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        {
            auto irr_shader = Shader::loadShader(
                cubemap_capture_vs_shader, irradiance_convolution_fs_shader);
            const GlShader* irr =
                dynamic_cast<const GlShader*>(irr_shader.get());

            irr->use();
            irr->setValue("environmentMap", 0);
            irr->setValue("projection", kCaptureProjection);
            irr->setValue("sourceLod",
                          std::log2(float(kCaptureFaceSize)
                                    / float(kIrradianceFaceSize)));
            irr->setValue("maxRadiance", ibl_radiance_clamp_);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_buffer_);

            glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                                  kIrradianceFaceSize, kIrradianceFaceSize);
            glViewport(0, 0, kIrradianceFaceSize, kIrradianceFaceSize);
            glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);

            for (unsigned int i = 0; i < 6; i++)
            {
                irr->setValue("view", kCaptureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                       irradiance_map_, 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
        GeneratePrefilterMap(cube_map_buffer_, prefilter_map_, skybox_vao_,
                             capture_fbo, capture_rbo, ibl_radiance_clamp_);
        GenerateBRDFLUT(brdf_lut_);

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_CULL_FACE);
        glViewport(0, 0, GraphicApi::viewport_res_x,
                   GraphicApi::viewport_res_y);

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

        Camera* cam = Camera::get_active_camera();
        if (!cam)
            return;

        const GlShader* sh =
            dynamic_cast<const GlShader*>(skybox_shader_.get());

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        sh->use();
        sh->setValue("view", glm::mat4(glm::mat3(cam->get_view_matrix_())));
        sh->setValue("projection", cam->get_projection_matrix_());
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
