#pragma once

#include "gl_shader.hpp"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>

#include "default_shaders.hpp"

namespace raphEngine::graphics
{
    template <>
    inline void GlShader::setData(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(id_, name.c_str()),
                    static_cast<int>(value));
    }

    template <>
    inline void GlShader::setData(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(id_, name.c_str()), value);
    }

    template <>
    inline void GlShader::setDataArray(const std::string& name, size_t count,
                             int* value) const
    {
        glUniform3iv(glGetUniformLocation(id_, name.c_str()), count, value);
    }
    
    template <>
    inline void GlShader::setData(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(id_, name.c_str()), value);
    }

    template <>
    inline void GlShader::setDataArray(const std::string& name, size_t count,
                               float* value) const
    {
        glUniform3fv(glGetUniformLocation(id_, name.c_str()), count, value);
    }

    template <>
    inline void GlShader::setData(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(id_, name.c_str()), 1, &value.x);
    }

    template <>
    inline void GlShader::setDataArray(const std::string& name, size_t count,
                              const glm::vec2* value) const
    {
        glUniform2fv(glGetUniformLocation(id_, name.c_str()), count, &value->x);
    }

    template <>
    inline void GlShader::setData(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(id_, name.c_str()), 1, &value.x);
    }

    template <>
    inline void GlShader::setDataArray(const std::string& name, size_t count,
                              const glm::vec3* value) const
    {
        glUniform3fv(glGetUniformLocation(id_, name.c_str()), count, &value->x);
    }

    template <>
    inline void GlShader::setData(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(id_, name.c_str()), 1, &value[0]);
    }

    template <>
    inline void GlShader::setData(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE,
                           &mat[0][0]);
    }
    
    template <>
    inline void GlShader::setData(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE,
                           &mat[0][0]);
    }
    
    template <>
    inline void GlShader::setData(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE,
                           &mat[0][0]);
    }

} // namespace raphEngine::graphics
