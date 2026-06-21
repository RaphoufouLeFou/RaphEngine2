#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include <vector>

#include "graphics/graphic_api.hpp"

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API OpenGL : public GraphicApi
    {
    public:
        void Init(const settings::Graphics& graphics_settings,
                  const std::string& window_name) override;
        void Render() override;
        bool Refresh() override;

        bool IsKeyPressed(int key) const override;

        glm::vec2 GetCursorPos() const override;
        void SetCursorPos(double x, double y) const override;
        bool GetMouseButtonPressed(int button) const override;
        void SetMouseVisibility(bool visible) const override;
        bool IsWindowFocused() const override;

    };
} // namespace raphEngine::graphics::ogl
