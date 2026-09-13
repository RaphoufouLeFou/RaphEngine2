#pragma once

#include <RaphEngine2/export.hpp>

#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace fs = std::filesystem;

namespace raphEngine::graphics
{
    struct ShaderStages
    {
        std::string vertex = "";
        std::string tessControl = "";
        std::string tessEval = "";
        std::string geometry = "";
        std::string fragment = "";

        bool operator==(const ShaderStages&) const = default;
    };

    struct ShaderFilePaths
    {
        fs::path vertex;
        fs::path tessControl;
        fs::path tessEval;
        fs::path geometry;
        fs::path fragment;
    };

    class RAPHENGINE_API Shader
    {
    public:
        Shader();
        virtual ~Shader() = default;

        virtual void use() const = 0;

        virtual void setValue(const std::string& name, bool value) const = 0;
        virtual void setValue(const std::string& name, int value) const = 0;
        virtual void setValue(const std::string& name, float value) const = 0;
        virtual void setValue(const std::string& name,
                              const glm::vec2& value) const = 0;
        virtual void setValue(const std::string& name,
                              const glm::vec3& value) const = 0;
        virtual void setValue(const std::string& name,
                              const glm::vec4& value) const = 0;
        virtual void setValue(const std::string& name,
                              const glm::mat2& value) const = 0;
        virtual void setValue(const std::string& name,
                              const glm::mat3& value) const = 0;
        virtual void setValue(const std::string& name,
                              const glm::mat4& value) const = 0;
        virtual void setValueArray(const std::string& name, size_t count,
                                   const int* array) const = 0;
        virtual void setValueArray(const std::string& name, size_t count,
                                   const float* array) const = 0;
        virtual void setValueArray(const std::string& name, size_t count,
                                   const glm::vec2* array) const = 0;
        virtual void setValueArray(const std::string& name, size_t count,
                                   const glm::vec3* array) const = 0;

        static std::shared_ptr<Shader>
        loadShader(const std::string& vShaderCode = "",
                   const std::string& fShaderCode = "",
                   const std::string& gShaderCode = "");

        static std::shared_ptr<Shader> loadShader(const ShaderStages& stages);

        static std::shared_ptr<Shader>
        loadShaderFromFile(const ShaderFilePaths& paths);
    };

} // namespace raphEngine::graphics
