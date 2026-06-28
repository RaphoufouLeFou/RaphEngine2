#include "graphics/ogl/opengl.hpp"

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
#include <iostream>

namespace raphEngine::graphics::ogl
{

    void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        (void)window;
        glViewport(0, 0, width, height);
        graphics::GraphicApi::res_x = width;
        graphics::GraphicApi::res_y = height;
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

        Logger::LogDebug("starting with a resolution of ", ResX, 'x', ResY);

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

        // glfwGetWindowSize(window, &ResX, &ResY);
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
    }

    void OpenGL::Render()
    {
        GLShadowRenderer::prepare_shadows();

        for (auto* object : render_pool)
        {
            object->render_shadow();
        }

        GLShadowRenderer::cleanup_shadows();

        glViewport(0, 0, res_x, res_y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto* object : render_pool)
        {
            object->render();
        }

        GLShadowRenderer::debug_draw_lights();
    }

    bool OpenGL::Refresh()
    {
        render_pool.clear();
        glfwSwapBuffers(window);
        glfwPollEvents();

        return glfwWindowShouldClose(window) == 0
            && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS;
    }

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
