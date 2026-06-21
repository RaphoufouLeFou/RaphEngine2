#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>

#define SCANCODE_MASK (1<<30)
#define SCANCODE_TO_KEYCODE(X)  (X | SCANCODE_MASK)

namespace raphEngine::inputs
{

    class RAPHENGINE_API Mouse
    {
    public:
        enum class MouseButton
        {
            LEFT = 0,
            MIDDLE = 2,
            RIGHT = 1,
        };

        static bool IsMouseButtonPressed(MouseButton button);
        static glm::vec2 GetMousePos();
        static double GetMouseScroll();
        static void SetMousePosition(double x, double y);
        static void SetMouseVisibility(bool visible);
        static bool IsMouseOnScreen();
        static bool IsWindowFocused();
    };
}
