#include "graphics/ogl/rmlui_renderer.hpp"
#include "logger/logger.hpp"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

namespace raphEngine::graphics::ogl
{
    void RmlUiRenderer::Init(GLFWwindow* window, int width, int height)
    {
        instance_ = this;
        window_ = window;

        // Constructed here, not as members -- RenderInterface_GL3's constructor
        // issues real GL calls (shader compilation), so it must not run until
        // after glewInit() has actually loaded the GL function pointers.
        system_interface_ = std::make_unique<SystemInterface_GLFW>();
        system_interface_->SetWindow(window);

        render_interface_ = std::make_unique<RenderInterface_GL3>();

        Rml::SetSystemInterface(system_interface_.get());
        Rml::SetRenderInterface(render_interface_.get());

        if (!Rml::Initialise())
        {
            Logger::LogError("Failed to initialise RmlUi");
            return;
        }

        context_ = Rml::CreateContext("main", Rml::Vector2i(width, height));
        if (!context_)
        {
            Logger::LogError("Failed to create RmlUi context");
            return;
        }
    }

    void RmlUiRenderer::Shutdown()
    {
        // Interfaces must stay alive until after Rml::Shutdown() per RmlUi's
        // docs.
        Rml::Shutdown();
        render_interface_.reset();
        system_interface_.reset();
    }

    void RmlUiRenderer::Update()
    {
        if (context_)
            context_->Update();
    }

    void RmlUiRenderer::Render()
    {
        if (!context_ || !render_interface_)
            return;

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        render_interface_->BeginFrame();
        context_->Render();
        render_interface_->EndFrame();

        glEnable(GL_DEPTH_TEST);
    }

    void RmlUiRenderer::Resize(int width, int height)
    {
        if (context_)
            context_->SetDimensions(Rml::Vector2i(width, height));

        if (render_interface_)
            render_interface_->SetViewport(width, height);
    }

    bool RmlUiRenderer::IsInputEnabled() const
    {
#ifdef EDITOR_BUILD
        return GraphicApi::viewport_focused;
#else
        return true;
#endif
    }
} // namespace raphEngine::graphics::ogl