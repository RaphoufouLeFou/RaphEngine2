#include "graphics/ogl/rmlui_renderer.hpp"
#include "logger/logger.hpp"

#include <GL/glew.h>
#include <RmlUi/Core.h>
#include <RmlUi_Platform_GLFW.h>
#include <RmlUi_Renderer_GL3.h>
#include <GLFW/glfw3.h>

namespace raphEngine::graphics::ogl
{
    struct RmlUiRenderer::Impl
    {
        std::unique_ptr<SystemInterface_GLFW> system_interface;
        std::unique_ptr<RenderInterface_GL3> render_interface;
    };

    RmlUiRenderer::RmlUiRenderer()
        : impl_(std::make_unique<Impl>())
    {}
    RmlUiRenderer::~RmlUiRenderer() = default;

    void RmlUiRenderer::Init(GLFWwindow* window, int width, int height)
    {
        instance_ = this;
        window_ = window;

        impl_->system_interface = std::make_unique<SystemInterface_GLFW>();
        impl_->system_interface->SetWindow(window);
        impl_->render_interface = std::make_unique<RenderInterface_GL3>();

        Rml::SetSystemInterface(impl_->system_interface.get());
        Rml::SetRenderInterface(impl_->render_interface.get());

        if (!Rml::Initialise())
        {
            Logger::LogError("Failed to initialise RmlUi");
            return;
        }

        context_ = Rml::CreateContext("main", Rml::Vector2i(width, height));
        if (!context_)
            Logger::LogError("Failed to create RmlUi context");
    }

    void RmlUiRenderer::Shutdown()
    {
        Rml::Shutdown();
        impl_->render_interface.reset();
        impl_->system_interface.reset();
    }

    void RmlUiRenderer::Update()
    {
        if (context_)
            context_->Update();
    }

    void RmlUiRenderer::Render()
    {
        if (!context_ || !impl_->render_interface)
            return;

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        impl_->render_interface->BeginFrame();
        context_->Render();
        impl_->render_interface->EndFrame();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

    void RmlUiRenderer::Resize(int width, int height)
    {
        if (context_)
            context_->SetDimensions(Rml::Vector2i(width, height));
        if (impl_->render_interface)
            impl_->render_interface->SetViewport(width, height);
    }

    void RmlUiRenderer::LoadFont(const std::string& path)
    {
        if (!Rml::LoadFontFace(path))
            Logger::LogError("Failed to load font: ", path);
    }

    Rml::ElementDocument* RmlUiRenderer::LoadDocument(const std::string& path)
    {
        if (!context_)
            return nullptr;

        Rml::ElementDocument* doc = context_->LoadDocument(path);
        if (!doc)
        {
            Logger::LogError("Failed to load RmlUi document: ", path);
            return nullptr;
        }
        doc->Show();
        return doc;
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