#include <RaphEngine2/ui/ui.hpp>
#include "graphics/ogl/rmlui_renderer.hpp"

namespace raphEngine
{
    void UI::LoadFont(const std::string& path)
    {
        if (graphics::ogl::RmlUiRenderer::instance_)
            graphics::ogl::RmlUiRenderer::instance_->LoadFont(path);
    }

    UIDocument UI::LoadDocument(const std::string& path)
    {
        if (!graphics::ogl::RmlUiRenderer::instance_)
            return UIDocument();

        void* doc = graphics::ogl::RmlUiRenderer::instance_->LoadDocument(path);
        return UIDocument(doc);
    }
} // namespace raphEngine