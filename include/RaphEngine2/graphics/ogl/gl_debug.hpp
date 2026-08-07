#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>

#include "graphics/debug.hpp"

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API GL_Debug : public Debug
    {
    public:
        void Init() override;

        void DrawLine(const Line line) override;
        void DrawLine(const glm::vec3& start, const glm::vec3& end,
                      const glm::vec3& color = { 1.0f, 1.0f, 0.0f },
                      const bool persistant = false) override;
        void RenderAllLines() override;

    private:
        void RenderLine(const glm::vec3& start, const glm::vec3& end,
                        const glm::vec3& color);
        uint line_vao, line_vbo;
    };
} // namespace raphEngine::graphics::ogl