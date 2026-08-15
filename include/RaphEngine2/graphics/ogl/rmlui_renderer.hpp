#pragma once
#include <RaphEngine2/export.hpp>
#include <memory>
#include <string>

struct GLFWwindow;

namespace Rml
{
    class Context;
    class ElementDocument;
} // namespace Rml

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API RmlUiRenderer
    {
    public:
        static inline RmlUiRenderer* instance_ = nullptr;

        RmlUiRenderer();
        ~RmlUiRenderer();

        void Init(GLFWwindow* window, int width, int height);
        void Shutdown();

        void Update();
        void Render();
        void Resize(int width, int height);

        void LoadFont(const std::string& path);
        Rml::ElementDocument* LoadDocument(const std::string& path);

        Rml::Context* GetContext() const
        {
            return context_;
        }
        bool IsInputEnabled() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        Rml::Context* context_ = nullptr;
        GLFWwindow* window_ = nullptr;
    };
} // namespace raphEngine::graphics::ogl