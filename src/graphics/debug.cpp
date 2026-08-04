#include "graphics/debug.hpp"

#include <RaphEngine2/export.hpp>
#include <memory>

#include "graphics/ogl/gl_debug.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"
#include "logger/logger.hpp"

namespace raphEngine::graphics
{
    std::unique_ptr<Debug> Debug::instance_ = nullptr;
    std::vector<Debug::Line> Debug::line_render_pool;
    std::vector<Debug::Line> Debug::persistant_line_render_pool;

    Debug* Debug::getInstance()
    {
        if (instance_ == nullptr)
        {
            if (Settings::Get<GraphicsSettings>().api == "OpenGL")
            {
                instance_ = std::make_unique<ogl::GL_Debug>();
                return instance_.get();
            }
            if (Settings::Get<GraphicsSettings>().api == "Vulkan")
            {
                Logger::LogError("Cannot get mesh renderer from Vulkan",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<ogl::GL_Debug>();
                return instance_.get();
                // TODO: for later
            }
            if (Settings::Get<GraphicsSettings>().api == "D3D11")
            {
                Logger::LogError("Cannot get mesh renderer from DirectX 11",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<ogl::GL_Debug>();
                return instance_.get();
                // TODO: for later
            }

            Logger::LogError(
                "Cannot get mesh renderer from an unknown grpahics API.",
                " Defaulting to OpenGl");

            instance_ = std::make_unique<ogl::GL_Debug>();
        }

        return instance_.get();
    }
} // namespace raphEngine::graphics
