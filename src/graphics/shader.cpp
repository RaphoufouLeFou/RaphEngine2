#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <graphics/ogl/gl_shader.hpp>
#include <graphics/shader.hpp>
#include <memory>
#include <string>
#include <vector>
#include "settings/graphics.hpp"
#include "settings/settings.hpp"
#include "logger/logger.hpp"

namespace raphEngine::graphics
{

    Shader::Shader()
    {
        // base class constructor - nothing to do
    }

    std::shared_ptr<Shader> Shader::loadShader(const std::string& vShaderCode,
                                               const std::string& fShaderCode,
                                               const std::string& gShaderCode)
    {
        if (Settings::Get<GraphicsSettings>().api == "OpenGL")
        {
            return GlShader::create_shader(vShaderCode, fShaderCode,
                                           gShaderCode);
        }
        if (Settings::Get<GraphicsSettings>().api == "Vulkan")
        {
            Logger::LogError("Cannot create Vulkan shader",
                             " (Not implemented) ", "Defaulting to OpenGl");
            return GlShader::create_shader(vShaderCode, fShaderCode,
                                           gShaderCode);
            // TODO: for later
        }
        if (Settings::Get<GraphicsSettings>().api == "D3D11")
        {
            Logger::LogError("Cannot create DirectX 11 shader",
                             " (Not implemented) ", "Defaulting to OpenGl");
            return GlShader::create_shader(vShaderCode, fShaderCode,
                                           gShaderCode);
            // TODO: for later
        }

        Logger::LogError("Cannot create shader for an unknown grpahics API",
                         " Defaulting to OpenGl");

        return GlShader::create_shader(vShaderCode, fShaderCode, gShaderCode);
    }

} // namespace raphEngine::graphics
