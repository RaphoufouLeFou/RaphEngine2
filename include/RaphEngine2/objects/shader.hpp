#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace raphEngine::objects
{
    class RAPHENGINE_API Shader
    {
    public:
        static std::shared_ptr<Shader>
        create_shader(const std::string& vShaderCode,
                      const std::string& fShaderCode,
                      const std::string& gShaderCode = "");

        Shader(const std::string& vShaderCode, const std::string& fShaderCode,
               const std::string& gShaderCode = "");

        void use();
        void setBool(const std::string& name, bool value) const;
        void setInt(const std::string& name, int value) const;
        void setIntArray(const std::string& name, int count, int* value) const;
        void setFloat(const std::string& name, float value) const;
        void setFloatArray(const std::string& name, int count,
                           float* value) const;
        void setVec2(const std::string& name, const glm::vec2& value) const;
        void setVec2(const std::string& name, float x, float y) const;
        void setVec2Array(const std::string& name, int count,
                          const glm::vec2* value) const;
        void setVec3(const std::string& name, const glm::vec3& value) const;
        void setVec3Array(const std::string& name, int count,
                          const glm::vec3* value) const;
        void setVec3(const std::string& name, float x, float y, float z) const;
        void setVec4(const std::string& name, const glm::vec4& value) const;
        void setVec4(const std::string& name, float x, float y, float z,
                     float w);
        void setMat2(const std::string& name, const glm::mat2& mat) const;
        void setMat3(const std::string& name, const glm::mat3& mat) const;
        void setMat4(const std::string& name, const glm::mat4& mat) const;
        void setModel(const glm::mat4& mat, int index) const;

    private:
        static std::vector<Shader*> loadedShaders_;
        unsigned int id_;
        void checkCompileErrors(unsigned int shader, const std::string& type);
    };
} // namespace raphEngine::objects
