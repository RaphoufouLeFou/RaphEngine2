#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include "graphics/shader.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GlShader : public Shader
    {
    public:
        static std::shared_ptr<GlShader>
        create_shader(const std::string& vShaderCode,
                      const std::string& fShaderCode,
                      const std::string& gShaderCode = "");

        GlShader(const std::string& vShaderCode, const std::string& fShaderCode,
               const std::string& gShaderCode = "");

        void use() override;

        void setValue(const std::string& name, bool value) const override;
        void setValue(const std::string& name, int value) const override;
        void setValue(const std::string& name, float value) const override;
        void setValue(const std::string& name, const glm::vec2& value) const override;
        void setValue(const std::string& name, const glm::vec3& value) const override;
        void setValue(const std::string& name, const glm::vec4& value) const override;
        void setValue(const std::string& name, const glm::mat2& value) const override;
        void setValue(const std::string& name, const glm::mat3& value) const override;
        void setValue(const std::string& name, const glm::mat4& value) const override;
        void setValueArray(const std::string& name, size_t count, const int* array) const override;
        void setValueArray(const std::string& name, size_t count, const float* array) const override;
        void setValueArray(const std::string& name, size_t count, const glm::vec2* array) const override;
        void setValueArray(const std::string& name, size_t count, const glm::vec3* array) const override;

    private:
        static std::vector<GlShader*> loadedShaders_;
        unsigned int id_;
        void checkCompileErrors(unsigned int shader, const std::string& type);
    };
} // namespace raphEngine::objects
