#include "graphics/ogl/opengl.hpp"
#include "graphics/debug.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <RaphEngine2/export.hpp>
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/renderable.hpp>
#include <RaphEngine2/settings/settings.hpp>
#include <RaphEngine2/graphics/ogl/gl_mesh_renderer.hpp>
#include <RaphEngine2/graphics/ogl/gl_shadow_renderer.hpp>

#ifdef EDITOR_BUILD
#    include "imgui_impl_glfw.h"
#    include "imgui_impl_opengl3.h"
#endif

namespace raphEngine::graphics::ogl
{

    void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        (void)window;
        glViewport(0, 0, width, height);
        GraphicApi::res_x = width;
        GraphicApi::res_y = height;

#ifdef EDITOR_BUILD
        GraphicApi::viewport_res_x = width; // <-- new
        GraphicApi::viewport_res_y = height; // <-- new
#endif
    }

    GLFWwindow* window;

    void SetHints()
    {
        glfwWindowHint(GLFW_SAMPLES, 8);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    void OpenGL::Init(const std::string& window_name)
    {
        if (!glfwInit())
        {
            Logger::LogError("Failed to initialize GLFW");
            exit(EXIT_FAILURE);
        }

        SetHints();
        instance_ = this;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        auto& gfx = Settings::Get<GraphicsSettings>();

        auto [ResX, ResY] = gfx.getResolution();
        res_x = ResX;
        res_y = ResY;

        window = glfwCreateWindow(ResX, ResY, window_name.c_str(),
                                  gfx.fullscreen ? monitor : NULL, NULL);

        if (!window)
        {
            Logger::LogError("Failed to create window (skill issue)");
            const char* description;
            int code = glfwGetError(&description);

            Logger::LogError("Error code: ", code,
                             ", description: ", description);
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        int x, y;
        glfwGetWindowSize(window, &x, &y);
        res_x = x;
        res_y = y;

        Logger::LogDebug("starting with a resolution of ", res_x, 'x', res_y);

        glfwMakeContextCurrent(window);

        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        glewExperimental = true; // Needed in core profile
        if (glewInit() != GLEW_OK)
        {
            Logger::LogError("Failed to initialize GLEW");
            exit(EXIT_FAILURE);
            return;
        }

        glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
        // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        glfwPollEvents();
        // glfwSetCursorPos(window, ResX / 2, ResY / 2);

        glClearColor(0.36f, 0.74f, 0.89f, 0.0f);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        // glEnable(GL_CULL_FACE);

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glfwSwapInterval(1);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glViewport(0, 0, ResX, ResY);

        GLShadowRenderer::generate_shadows_buffer();

#ifdef EDITOR_BUILD
        ImGui_ImplGlfw_InitForOpenGL(
            window,
            true); // Second param install_callback=true will install GLFW
                   // callbacks and chain to existing ones.
        Logger::LogDebug("imgui opengl3 init");
        ImGui_ImplOpenGL3_Init();
        CreateViewportFramebuffer(res_x, res_y);

#endif
    }

    void OpenGL::StartFrame()
    {
#ifdef EDITOR_BUILD
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
#endif
    }

    void OpenGL::Render()
    {
        GLShadowRenderer::prepare_shadows();
        if (ShadowRenderer::GetDirectionalLight()->cast_shadows_)
        {
            for (auto* object : render_pool)
                object->render_shadow();
        }
        GLShadowRenderer::cleanup_shadows();

#ifdef EDITOR_BUILD
        glBindFramebuffer(GL_FRAMEBUFFER, viewport_fbo_ms_);
#endif
        glViewport(0, 0, res_x, res_y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto* object : render_pool)
            object->render();

        GLShadowRenderer::debug_draw_lights();

        Debug::getInstance()->RenderAllLines();

#ifdef EDITOR_BUILD
        glBindFramebuffer(GL_READ_FRAMEBUFFER, viewport_fbo_ms_);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_fbo_resolve_);
        glBlitFramebuffer(0, 0, viewport_width_, viewport_height_, 0, 0,
                          viewport_width_, viewport_height_,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
    }

    bool OpenGL::Refresh()
    {
#ifdef EDITOR_BUILD
        glViewport(0, 0, res_x, res_y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
        render_pool.clear();
        lights_pool.clear();
        spot_lights_pool.clear();
        glfwSwapBuffers(window);
        glfwPollEvents();

        bool stay_open = glfwWindowShouldClose(window) == 0
            && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS;

        if (!stay_open)
        {
#ifdef EDITOR_BUILD
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
#endif
        }

        return stay_open;
    }

#ifdef EDITOR_BUILD
    void OpenGL::CreateViewportFramebuffer(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return;

        viewport_width_ = width;
        viewport_height_ = height;

        GraphicApi::res_x = width;
        GraphicApi::res_y = height;

        glGenFramebuffers(1, &viewport_fbo_ms_);
        glBindFramebuffer(GL_FRAMEBUFFER, viewport_fbo_ms_);

        glGenRenderbuffers(1, &viewport_color_rbo_ms_);
        glBindRenderbuffer(GL_RENDERBUFFER, viewport_color_rbo_ms_);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, viewport_samples_,
                                         GL_RGB8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_RENDERBUFFER, viewport_color_rbo_ms_);

        glGenRenderbuffers(1, &viewport_depth_rbo_ms_);
        glBindRenderbuffer(GL_RENDERBUFFER, viewport_depth_rbo_ms_);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, viewport_samples_,
                                         GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, viewport_depth_rbo_ms_);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            Logger::LogError("Viewport MSAA framebuffer is not complete!");

        glGenFramebuffers(1, &viewport_fbo_resolve_);
        glBindFramebuffer(GL_FRAMEBUFFER, viewport_fbo_resolve_);

        glGenTextures(1, &viewport_color_tex_);
        glBindTexture(GL_TEXTURE_2D, viewport_color_tex_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, viewport_color_tex_, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            Logger::LogError("Viewport resolve framebuffer is not complete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGL::ResizeViewportFramebuffer(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return;
        if (width == viewport_width_ && height == viewport_height_)
            return;

        if (viewport_fbo_ms_)
        {
            glDeleteFramebuffers(1, &viewport_fbo_ms_);
            glDeleteRenderbuffers(1, &viewport_color_rbo_ms_);
            glDeleteRenderbuffers(1, &viewport_depth_rbo_ms_);
            glDeleteFramebuffers(1, &viewport_fbo_resolve_);
            glDeleteTextures(1, &viewport_color_tex_);
        }
        CreateViewportFramebuffer(width, height);
    }
#endif

    bool OpenGL::IsKeyPressed(int key) const
    {
        return glfwGetKey(window, (int)key) == GLFW_PRESS;
    };

    glm::vec2 OpenGL::GetCursorPos() const
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        return glm::vec2(x, y);
    }

    void OpenGL::SetCursorPos(double x, double y) const
    {
        glfwSetCursorPos(window, x, y);
    }

    bool OpenGL::GetMouseButtonPressed(int button) const
    {
        return glfwGetMouseButton(window, button);
    }

    void OpenGL::SetMouseVisibility(bool visible) const
    {
        if (visible)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    bool OpenGL::IsWindowFocused() const
    {
        return glfwGetWindowAttrib(window, GLFW_FOCUSED);
    }

} // namespace raphEngine::graphics::ogl
