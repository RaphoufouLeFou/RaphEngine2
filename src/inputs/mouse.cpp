#include <RaphEngine2/graphics/graphic_api.hpp>
#include <RaphEngine2/inputs/mouse.hpp>

namespace raphEngine::inputs
{
    bool Mouse::IsMouseButtonPressed(MouseButton button)
    {
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
        unsigned short resX = graphics::GraphicApi::res_x;
        unsigned short resY = graphics::GraphicApi::res_y;
        return (pos.x >= 0 && pos.x <= resX && pos.y >= 0 && pos.y <= resY);
    }

    bool Mouse::IsWindowFocused()
    {
        return graphics::GraphicApi::get_api()->IsWindowFocused();
    }
} // namespace raphEngine::inputs
