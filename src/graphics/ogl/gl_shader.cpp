#include "graphics/ogl/gl_shader.hpp"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>

#include "default_shaders.hpp"

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

    void GlShader::use()
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
                std::cout
                    << "ERROR::SHADER_COMPILATION_ERROR of type: " << type
                    << "\n"
                    << infoLog
                    << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout
                    << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                    << infoLog
                    << std::endl;
            }
        }
    }
} // namespace raphEngine::graphics
