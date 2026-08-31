#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include "RaphEngine2/graphics/ogl/rmlui_renderer.hpp"
#include "graphics/graphic_api.hpp"
#include "graphics/ogl/rmlui_renderer.hpp"

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API OpenGL : public GraphicApi
    {
    public:
        void Init(const std::string& window_name) override;
        void StartFrame() override;
        void Render() override;
        bool Refresh() override;

        bool IsKeyPressed(int key) const override;
        glm::vec2 GetCursorPos() const override;
        void SetCursorPos(double x, double y) const override;
        bool GetMouseButtonPressed(int button) const override;
        void SetMouseVisibility(bool visible) const override;
        bool IsWindowFocused() const override;
        void RequestQuit() const override;
        void ShowStartupScreen() const override;

        RmlUiRenderer& GetRmlUiRenderer()
        {
            return rmlui_renderer_;
        }

        void ResizeViewportFramebuffer(int width, int height) override;
        void* GetViewportTexture() const override
        {
            return (void*)(intptr_t)viewport_color_tex_;
        }

        graphics::ogl::RmlUiRenderer rmlui_renderer_;

    private:
        void CreateViewportFramebuffer(int width, int height);

        // Multisampled target we actually render into
        unsigned int viewport_fbo_ms_ = 0;
        unsigned int viewport_color_rbo_ms_ = 0;
        unsigned int viewport_depth_rbo_ms_ = 0;

        // Resolved single-sample target ImGui samples from
        unsigned int viewport_fbo_resolve_ = 0;
        unsigned int viewport_color_tex_ = 0;

        int viewport_width_ = 0;
        int viewport_height_ = 0;
        int viewport_samples_ = 4; // match whatever GLFW_SAMPLES you were using
    };
} // namespace raphEngine::graphics::ogl
