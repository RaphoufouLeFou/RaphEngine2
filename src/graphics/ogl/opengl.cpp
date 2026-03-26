#include "graphics/ogl/opengl.hpp"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include <iostream>

namespace raphEngine::graphics::ogl
{

    GLFWwindow* window;
    void OpenGL::Init(const settings::Graphics& graphics_settings,
                      const std::string& window_name)
    {
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            exit(EXIT_FAILURE);
        }

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        auto [ResX, ResY] = graphics_settings.getResolution();

        std::cout << "starting with a resolution of " << ResX << 'x' << ResY
                  << std::endl;

        window = glfwCreateWindow(
            ResX, ResY, window_name.c_str(),
            graphics_settings.getFullScreen() ? monitor : NULL, NULL);

        if (!window)
        {
            std::cerr << "Failed to create window (skill issue)" << std::endl;
            // display the error message
            const char* description;
            int code = glfwGetError(&description);
            std::cerr << "Error code: " << code
                      << ", description: " << description << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        // glfwGetWindowSize(window, &ResX, &ResY);
        glfwMakeContextCurrent(window); // Initialize GLEW

        // glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        glewExperimental = true; // Needed in core profile
        if (glewInit() != GLEW_OK)
        {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }

        std::cout << "Here" << std::endl;

        // Ensure we can capture the escape key being pressed below
        glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
        // Hide the mouse and enable unlimited movement
        // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Set the mouse at the center of the screen
        glfwPollEvents();
        // glfwSetCursorPos(window, ResX / 2, ResY / 2);

        // Dark blue background
        glClearColor(0.36f, 0.74f, 0.89f, 0.0f);

        // Enable depth test
        glEnable(GL_DEPTH_TEST);
        // Accept fragment if it is closer to the camera than the former one
        glDepthFunc(GL_LESS);
        // Cull triangles which normal is not towards the camera
        // glEnable(GL_CULL_FACE);

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void OpenGL::Render()
    {}

    void OpenGL::AddToRenderPool(const Renderable& renderable)
    {
        render_pool.push_back(&renderable);
    }

    void OpenGL::Refresh()
    {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

} // namespace raphEngine::graphics::ogl
