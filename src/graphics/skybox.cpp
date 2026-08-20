#include <RaphEngine2/graphics/skybox.hpp>
#include <RaphEngine2/graphics/ogl/gl_skybox.hpp>
#include <memory>

#include "logger/logger.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"
namespace raphEngine::graphics
{

    std::unique_ptr<Skybox> Skybox::instance_ = nullptr;
    Skybox* Skybox::getInstance()
    {
        if (instance_ == nullptr)
        {
            if (Settings::Get<GraphicsSettings>().api == "OpenGL")
            {
                instance_ = std::make_unique<ogl::GL_Skybox>();
                return instance_.get();
            }
            if (Settings::Get<GraphicsSettings>().api == "Vulkan")
            {
                Logger::LogError("Cannot get skybox renderer from Vulkan",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<ogl::GL_Skybox>();
                return instance_.get();
                // TODO: for later
            }
            if (Settings::Get<GraphicsSettings>().api == "D3D11")
            {
                Logger::LogError("Cannot get skybox renderer from DirectX 11",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<ogl::GL_Skybox>();
                return instance_.get();
                // TODO: for later
            }

            Logger::LogError(
                "Cannot get skybox renderer from an unknown grpahics API.",
                " Defaulting to OpenGl");

            instance_ = std::make_unique<ogl::GL_Skybox>();
        }

        return instance_.get();
    }

} // namespace raphEngine::graphics
