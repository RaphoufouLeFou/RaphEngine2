#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>

namespace raphEngine::graphics
{
    class RAPHENGINE_API Debug
    {
    public:
        struct Line
        {
            bool persistant;
            const glm::vec3 start;
            const glm::vec3 end;
            const glm::vec3 color;
        };

        virtual void Init() = 0;
        virtual void DrawLine(const Line line) = 0;
        virtual void DrawLine(const glm::vec3& start, const glm::vec3& end,
                              const glm::vec3& color = { 1.0f, 1.0f, 0.0f },
                              const bool persistant = false) = 0;

        virtual void RenderAllLines() = 0;
        static Debug* getInstance();

    protected:
        static std::vector<Line> line_render_pool;
        static std::vector<Line> persistant_line_render_pool;

    private:
        static std::unique_ptr<Debug> instance_;
    };
} // namespace raphEngine::graphics