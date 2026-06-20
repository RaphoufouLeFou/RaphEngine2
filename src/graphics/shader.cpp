#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <graphics/shader.hpp>
#include <graphics/ogl/gl_shader.hpp>

namespace raphEngine::graphics
{

        Shader::Shader()
        {
            // base class constructor - nothing to do
        }


        std::shared_ptr<Shader> Shader::loadShader(
            const std::string& vShaderCode,
            const std::string& fShaderCode,
            const std::string& gShaderCode
        )
        {
            // TODO: select the right api
            return GlShader::create_shader(vShaderCode, fShaderCode, gShaderCode);
        }

} // namespace raphEngine::objects
