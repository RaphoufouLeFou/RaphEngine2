#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include "ogl/gl_shader.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API Shader
    {
    public:
        Shader();

        virtual void use() = 0;

        virtual void setValue(const std::string& name, bool value) const = 0;
        virtual void setValue(const std::string& name, int value) const = 0;
        virtual void setValue(const std::string& name, float value) const = 0;
        virtual void setValue(const std::string& name, const glm::vec2& value) const = 0;
        virtual void setValue(const std::string& name, const glm::vec3& value) const = 0;
        virtual void setValue(const std::string& name, const glm::vec4& value) const = 0;
        virtual void setValue(const std::string& name, const glm::mat2& value) const = 0;
        virtual void setValue(const std::string& name, const glm::mat3& value) const = 0;
        virtual void setValue(const std::string& name, const glm::mat4& value) const = 0;
        virtual void setValueArray(const std::string& name, size_t count, const int* array) const = 0;
        virtual void setValueArray(const std::string& name, size_t count, const float* array) const = 0;
        virtual void setValueArray(const std::string& name, size_t count, const glm::vec2* array) const = 0;
        virtual void setValueArray(const std::string& name, size_t count, const glm::vec3* array) const = 0;

        std::shared_ptr<Shader> loadShader(
            const std::string& vShaderCode,
            const std::string& fShaderCode,
            const std::string& gShaderCode = ""
        );
    };

} // namespace raphEngine::objects
