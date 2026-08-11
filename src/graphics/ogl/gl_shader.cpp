#include "graphics/ogl/gl_shader.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

#include "default_shaders.hpp"
#include <RaphEngine2/logger/logger.hpp>

namespace raphEngine::graphics
{
    std::vector<GlShader*> GlShader::loadedShaders_ = std::vector<GlShader*>();

    std::shared_ptr<GlShader>
    GlShader::create_shader(const std::string& vShaderCode,
                            const std::string& fShaderCode,
                            const std::string& gShaderCode)
    {
        return std::make_shared<GlShader>(
            vShaderCode.empty() ? default_vs_shader : vShaderCode,
            fShaderCode.empty() ? default_fs_shader : fShaderCode, gShaderCode);
    }

    GlShader::GlShader(const std::string& vShaderCode,
                       const std::string& fShaderCode,
                       const std::string& gShaderCode)
    {
        unsigned int vertex, fragment;

        Logger::LogDebug("compiling a shader");

        vertex = glCreateShader(GL_VERTEX_SHADER);
        const char* vertexCode = vShaderCode.c_str();
        const char* fragmentCode = fShaderCode.c_str();
        glShaderSource(vertex, 1, &vertexCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        unsigned int geometry = 0;
        if (!gShaderCode.empty())
        {
            const char* geometryCode = gShaderCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &geometryCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }

        id_ = glCreateProgram();
        glAttachShader(id_, vertex);
        glAttachShader(id_, fragment);
        if (!gShaderCode.empty())
            glAttachShader(id_, geometry);
        glLinkProgram(id_);
        checkCompileErrors(id_, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (!gShaderCode.empty())
            glDeleteShader(geometry);
        GlShader::loadedShaders_.push_back(this);
    }

    void GlShader::use() const
    {
        glUseProgram(id_);
    }

    void GlShader::checkCompileErrors(GLuint shader, const std::string& type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                Logger::LogError("shader compilation error of type ", type,
                                 "\n", infoLog);
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                Logger::LogError("shader linking error of type ", type, "\n",
                                 infoLog);
            }
        }
    }

    GLint GlShader::getUniformLocation(const std::string& name) const
    {
        if (auto it = uniform_location_cache_.find(name);
            it != uniform_location_cache_.end())
            return it->second;

        GLint location = glGetUniformLocation(id_, name.c_str());
        uniform_location_cache_[name] =
            location; // caches -1 too — an
                      // absent uniform is just as stable as a present one
        return location;
    }

    void GlShader::setValue(const std::string& name, bool value) const
    {
        glUniform1i(getUniformLocation(name), static_cast<int>(value));
    }

    void GlShader::setValue(const std::string& name, int value) const
    {
        glUniform1i(getUniformLocation(name), value);
    }

    void GlShader::setValue(const std::string& name, float value) const
    {
        glUniform1f(getUniformLocation(name), value);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::vec2& value) const
    {
        glUniform2fv(getUniformLocation(name), 1, &value.x);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::vec3& value) const
    {
        glUniform3fv(getUniformLocation(name), 1, &value.x);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::vec4& value) const
    {
        glUniform4fv(getUniformLocation(name), 1, &value[0]);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::mat2& value) const
    {
        glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::mat3& value) const
    {
        glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::mat4& value) const
    {
        glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }

    void GlShader::setValueArray(const std::string& name, size_t count,
                                 const int* array) const
    {
        glUniform3iv(getUniformLocation(name), count, array);
    }

    void GlShader::setValueArray(const std::string& name, size_t count,
                                 const float* array) const
    {
        glUniform3fv(getUniformLocation(name), count, array);
    }

    void GlShader::setValueArray(const std::string& name, size_t count,
                                 const glm::vec2* array) const
    {
        glUniform2fv(getUniformLocation(name), count, &array->x);
    }

    void GlShader::setValueArray(const std::string& name, size_t count,
                                 const glm::vec3* array) const
    {
        glUniform3fv(getUniformLocation(name), count, &array->x);
    }

} // namespace raphEngine::graphics