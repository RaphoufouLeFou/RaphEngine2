#include "graphics/graphic_api.hpp"
#include "graphics/ogl/rmlui_renderer.hpp"
#include <RmlUi_Platform_GLFW.h>

namespace raphEngine::inputs
{
    static int GetCurrentGlfwMods(GLFWwindow* window)
    {
        int mods = 0;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            mods |= GLFW_MOD_SHIFT;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
            mods |= GLFW_MOD_CONTROL;
        if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
            mods |= GLFW_MOD_ALT;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS)
            mods |= GLFW_MOD_SUPER;
        return mods;
    }

    void rmlui_cursor_pos_callback(GLFWwindow* window, double x, double y)
    {
        auto* r = graphics::ogl::RmlUiRenderer::instance_;
        if (!r || !r->GetContext() || !r->IsInputEnabled())
            return;

#ifdef EDITOR_BUILD
        x -= graphics::GraphicApi::viewport_pos_x;
        y -= graphics::GraphicApi::viewport_pos_y;
#endif
        int mods = RmlGLFW::ConvertKeyModifiers(GetCurrentGlfwMods(window));
        RmlGLFW::ProcessCursorPosCallback(r->GetContext(), window, x, y, mods);
    }

    void rmlui_mouse_button_callback(GLFWwindow* window, int button, int action,
                                     int mods)
    {
        (void)window;
        auto* r = graphics::ogl::RmlUiRenderer::instance_;
        if (!r || !r->GetContext() || !r->IsInputEnabled())
            return;

        int rml_mods = RmlGLFW::ConvertKeyModifiers(mods);
        RmlGLFW::ProcessMouseButtonCallback(r->GetContext(), button, action,
                                            rml_mods);
    }

    void rmlui_scroll_callback(GLFWwindow* window, double xoffset,
                               double yoffset)
    {
        (void)xoffset;
        auto* r = graphics::ogl::RmlUiRenderer::instance_;
        if (!r || !r->GetContext() || !r->IsInputEnabled())
            return;

        int mods = RmlGLFW::ConvertKeyModifiers(GetCurrentGlfwMods(window));
        RmlGLFW::ProcessScrollCallback(r->GetContext(), yoffset, mods);
    }

    void rmlui_key_callback(GLFWwindow* window, int key, int scancode,
                            int action, int mods)
    {
        (void)window;
        (void)scancode;
        auto* r = graphics::ogl::RmlUiRenderer::instance_;
        if (!r || !r->GetContext() || !r->IsInputEnabled())
            return;

        int rml_mods = RmlGLFW::ConvertKeyModifiers(mods);
        RmlGLFW::ProcessKeyCallback(r->GetContext(), key, action, rml_mods);
    }

    void rmlui_char_callback(GLFWwindow* window, unsigned int codepoint)
    {
        (void)window;
        auto* r = graphics::ogl::RmlUiRenderer::instance_;
        if (!r || !r->GetContext() || !r->IsInputEnabled())
            return;

        RmlGLFW::ProcessCharCallback(r->GetContext(), codepoint);
    }
} // namespace raphEngine::inputs