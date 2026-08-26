#include <RaphEngine2/export.hpp>
#include <RaphEngine2/renderable.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "component/camera_component.hpp"
#include "graphics/shader.hpp"

#include "default_shaders.hpp"
#include "graphics/ogl/gl_debug.hpp"

namespace raphEngine::graphics::ogl
{

    void GL_Debug::Init()
    {
        glGenVertexArrays(1, &line_vao);
        glGenBuffers(1, &line_vbo);

        glBindVertexArray(line_vao);
        glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 2, nullptr,
                     GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                              (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void GL_Debug::DrawLine(const Line line)
    {
        if (line.persistant)
            persistant_line_render_pool.push_back(line);
        else
            line_render_pool.push_back(line);
    }
    void GL_Debug::DrawLine(const glm::vec3& start, const glm::vec3& end,
                            const glm::vec3& color, const bool persistant)
    {
        Line l = {
            .persistant = persistant, .start = start, .end = end, .color = color
        };

        DrawLine(l);
    }
    void GL_Debug::RenderAllLines()
    {
        for (auto& l : persistant_line_render_pool)
        {
            RenderLine(l.start, l.end, l.color);
        }
        for (auto l : line_render_pool)
        {
            RenderLine(l.start, l.end, l.color);
        }

        line_render_pool.clear();
    }

    void GL_Debug::RenderLine(const glm::vec3& start, const glm::vec3& end,
                              const glm::vec3& color)
    {
        glm::vec3 points[2] = { start, end };

        glBindVertexArray(line_vao);
        glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);

        static std::shared_ptr<Shader> line_shader = nullptr;

        if (!line_shader)
        {
            line_shader =
                Shader::loadShader(debug_line_vs_shader, debug_line_fs_shader);
        }

        line_shader->use();
        line_shader->setValue(
            "u_MVP",
            Camera::get_active_camera()->get_projection_matrix_()
                * Camera::get_active_camera()->get_view_matrix_());

        line_shader->setValue("u_Color", color);

        glDrawArrays(GL_LINES, 0, 2);

        glBindVertexArray(0);
    }
} // namespace raphEngine::graphics::ogl
