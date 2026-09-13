#pragma once

#include <RaphEngine2/export.hpp>
#include <RaphEngine2/graphics/shader.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

namespace raphEngine::graphics
{
    class RAPHENGINE_API GlShader final : public Shader
    {
    public:
        GlShader(const std::string& vShaderCode, const std::string& fShaderCode,
                 const std::string& gShaderCode = "");
        explicit GlShader(const ShaderStages& stages);

        static std::shared_ptr<GlShader>
        create_shader(const std::string& vShaderCode,
                      const std::string& fShaderCode,
                      const std::string& gShaderCode = "");
        static std::shared_ptr<GlShader>
        create_shader(const ShaderStages& stages);

        void use() const override;

        void setValue(const std::string& name, bool value) const override;
        void setValue(const std::string& name, int value) const override;
        void setValue(const std::string& name, float value) const override;
        void setValue(const std::string& name,
                      const glm::vec2& value) const override;
        void setValue(const std::string& name,
                      const glm::vec3& value) const override;
        void setValue(const std::string& name,
                      const glm::vec4& value) const override;
        void setValue(const std::string& name,
                      const glm::mat2& value) const override;
        void setValue(const std::string& name,
                      const glm::mat3& value) const override;
        void setValue(const std::string& name,
                      const glm::mat4& value) const override;
        void setValueArray(const std::string& name, size_t count,
                           const int* array) const override;
        void setValueArray(const std::string& name, size_t count,
                           const float* array) const override;
        void setValueArray(const std::string& name, size_t count,
                           const glm::vec2* array) const override;
        void setValueArray(const std::string& name, size_t count,
                           const glm::vec3* array) const override;

        static std::vector<GlShader*> loadedShaders_;

    private:
        int getUniformLocation(const std::string& name) const;

        static unsigned int CompileStage(const std::string& source,
                                         unsigned int stageType,
                                         const std::string& stageLabel);
        static void checkCompileErrors(unsigned int shader,
                                       const std::string& type);

        unsigned int id_ = 0;
        mutable std::unordered_map<std::string, int> uniform_location_cache_;
    };
} // namespace raphEngine::graphics
