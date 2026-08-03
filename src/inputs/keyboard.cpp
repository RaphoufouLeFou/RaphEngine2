#include <RaphEngine2/graphics/graphic_api.hpp>
#include <RaphEngine2/inputs/keyboard.hpp>

namespace raphEngine::inputs
{
    bool Key::IsKeyPressed(KeyCode key)
    {
#ifdef EDITOR_BUILD
        if (!graphics::GraphicApi::viewport_focused)
            return false;
#endif

        bool isPressed =
            graphics::GraphicApi::get_api()->IsKeyPressed((int)key);
        return isPressed;
    }
} // namespace raphEngine::inputs
