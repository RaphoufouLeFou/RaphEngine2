#include <RaphEngine2/graphics/outline_renderer.hpp>
#include <RaphEngine2/graphics/ogl/gl_outline_renderer.hpp>
#include "settings/graphics.hpp"
#include "settings/settings.hpp"
#include "logger/logger.hpp"

namespace raphEngine::graphics
{
    std::unique_ptr<OutlineRenderer> OutlineRenderer::instance_ = nullptr;

    OutlineRenderer* OutlineRenderer::getInstance()
    {
        if (instance_ == nullptr)
        {
            if (Settings::Get<GraphicsSettings>().api != "OpenGL")
                Logger::LogError("Cannot get OutlineRenderer for this API",
                                 " (Not implemented). Defaulting to OpenGl");
            instance_ = std::make_unique<ogl::GL_OutlineRenderer>();
        }
        return instance_.get();
    }
} // namespace raphEngine::graphics
