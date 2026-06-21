#include <RaphEngine2/inputs/keyboard.hpp>

#include <RaphEngine2/graphics/graphic_api.hpp>

namespace raphEngine::inputs
{
    bool Key::IsKeyPressed(KeyCode key) {
        bool isPressed = graphics::GraphicApi::get_api()->IsKeyPressed((int)key);
        return isPressed;
    }
}
