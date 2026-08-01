#include <RaphEngine2/graphics/graphic_api.hpp>
#include <RaphEngine2/inputs/keyboard.hpp>

#include "imgui.h"

namespace raphEngine::inputs
{
    bool Key::IsKeyPressed(KeyCode key)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard)
            return false;

        bool isPressed =
            graphics::GraphicApi::get_api()->IsKeyPressed((int)key);
        return isPressed;
    }
} // namespace raphEngine::inputs
