#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include "ogl/gl_shader.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API ShadersManager
    {
    public:
        ShadersManager(const std::string& vShaderCode,
            const std::string& fShaderCode,
            const std::string& gShaderCode = "");

        void use();

        template <typename T>
        void setValue(const std::string& name, T value);

        template <typename T>
        void setValueArray(const std::string& name, size_t count, T value);

    private:
        static std::vector<GlShader*> loadedGlShaders_;
        // TODO
        // static std::vector<VkShader*> loadedVkShaders_;
        // static std::vector<DxShader*> loadedDxShaders_;

        std::shared_ptr<GlShader> currentGlShaders_;

    };
} // namespace raphEngine::objects
