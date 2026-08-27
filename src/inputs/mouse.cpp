#include <RaphEngine2/graphics/graphic_api.hpp>
#include <RaphEngine2/inputs/mouse.hpp>
#include <RaphEngine2/core.hpp>

namespace raphEngine::inputs
{
    bool Mouse::IsMouseButtonPressed(MouseButton button)
    {
        if (Core::is_editor_mode())
        {
            if (!graphics::GraphicApi::viewport_focused)
                return false;
        }

        return graphics::GraphicApi::get_api()->GetMouseButtonPressed(
            (int)button);
    }

    glm::vec2 Mouse::GetMousePos()
    {
        return graphics::GraphicApi::get_api()->GetCursorPos();
    }

    double Mouse::GetMouseScroll()
    {
        // TODO: get the mouse scroll
        return 0;
    }

    void Mouse::SetMousePosition(double x, double y)
    {
        graphics::GraphicApi::get_api()->SetCursorPos(x, y);
    }

    void Mouse::SetMouseVisibility(bool visible)
    {
        graphics::GraphicApi::get_api()->SetMouseVisibility(visible);
    }

    bool Mouse::IsMouseOnScreen()
    {
        glm::vec2 pos = graphics::GraphicApi::get_api()->GetCursorPos();
        unsigned short resX = graphics::GraphicApi::viewport_res_x;
        unsigned short resY = graphics::GraphicApi::viewport_res_y;

        unsigned short posX = graphics::GraphicApi::viewport_pos_x;
        unsigned short posY = graphics::GraphicApi::viewport_pos_y;

        return (pos.x >= posX && pos.x <= posX + resX && pos.y >= posY
                && pos.y <= posY + resY);
    }

    bool Mouse::IsWindowFocused()
    {
        return graphics::GraphicApi::get_api()->IsWindowFocused();
    }
} // namespace raphEngine::inputs
