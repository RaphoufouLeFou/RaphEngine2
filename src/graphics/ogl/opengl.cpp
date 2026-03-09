#include <RaphEngine2/export.hpp>
#include "graphics/ogl/opengl.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <iostream>

namespace raphEngine::graphics::ogl {

    void OpenGL::Init(const settings::Graphics& graphics_settings, const std::string& window_name)
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        int ResX = glfwGetVideoMode(monitor)->width;
        int ResY = glfwGetVideoMode(monitor)->height;

        GLFWwindow* window = glfwCreateWindow(ResX, ResY, window_name.c_str(), graphics_settings.getFullScreen() ? monitor : NULL, NULL);

        if (!window) {
            std::cerr << "Failed to create window (skill issue)" << std::endl;
            // display the error message
            const char* description;
            int code = glfwGetError(&description);
            std::cerr << "Error code: " << code << ", description: " << description << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }


        glfwGetWindowSize(window, &ResX, &ResY);
        glfwMakeContextCurrent(window); // Initialize GLEW

        //glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        glewExperimental = true; // Needed in core profile
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            exit(EXIT_FAILURE);
            return;
        }
    }
    void OpenGL::Render(const Renderable& renderable)
    {
        (void) renderable;
    }

}
