#pragma once

#include <RaphEngine2/export.hpp>
#include <GL/glew.h>
#include <RmlUi/Core.h>
#include <RmlUi_Platform_GLFW.h>
#include <RmlUi_Renderer_GL3.h>
#include <GLFW/glfw3.h>
#include <memory>

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API RmlUiRenderer
    {
    public:
        static inline RmlUiRenderer* instance_ = nullptr;

        RmlUiRenderer() = default;
        ~RmlUiRenderer() = default;

        void Init(GLFWwindow* window, int width, int height);
        void Shutdown();

        void Update();
        void Render();
        void Resize(int width, int height);

        Rml::Context* GetContext() const
        {
            return context_;
        }
        bool IsInputEnabled() const;

    private:
        std::unique_ptr<SystemInterface_GLFW> system_interface_;
        std::unique_ptr<RenderInterface_GL3> render_interface_;
        Rml::Context* context_ = nullptr;
        GLFWwindow* window_ = nullptr;
    };
} // namespace raphEngine::graphics::ogl