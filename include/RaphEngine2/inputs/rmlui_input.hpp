#pragma once
#include <GLFW/glfw3.h>

namespace raphEngine::inputs
{
    void rmlui_cursor_pos_callback(GLFWwindow* window, double x, double y);
    void rmlui_mouse_button_callback(GLFWwindow* window, int button, int action,
                                     int mods);
    void rmlui_scroll_callback(GLFWwindow* window, double xoffset,
                               double yoffset);
    void rmlui_key_callback(GLFWwindow* window, int key, int scancode,
                            int action, int mods);
    void rmlui_char_callback(GLFWwindow* window, unsigned int codepoint);
} // namespace raphEngine::inputs