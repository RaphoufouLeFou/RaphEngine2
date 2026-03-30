#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace raphEngine::graphics
{
    class RAPHENGINE_API GlShader
    {
    public:
        static std::shared_ptr<GlShader>
        create_shader(const std::string& vShaderCode,
                      const std::string& fShaderCode,
                      const std::string& gShaderCode = "");

        GlShader(const std::string& vShaderCode, const std::string& fShaderCode,
               const std::string& gShaderCode = "");

        void use();

        template <typename T>
        void setData(const std::string& name, T value) const;
        
        template <typename T>
        void setDataArray(const std::string& name, size_t count, T value) const;

        void setModel(const glm::mat4& mat, int index) const;

    private:
        static std::vector<GlShader*> loadedShaders_;
        unsigned int id_;
        void checkCompileErrors(unsigned int shader, const std::string& type);
    };
} // namespace raphEngine::objects

#include "gl_shader.hxx"
