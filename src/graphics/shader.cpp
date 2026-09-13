#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <graphics/ogl/gl_shader.hpp>
#include <graphics/shader.hpp>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include "default_shaders.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"
#include "logger/logger.hpp"

namespace raphEngine::graphics
{
    Shader::Shader()
    {}

    namespace
    {
        struct ShaderStagesHash
        {
            size_t operator()(const ShaderStages& k) const
            {
                size_t h = std::hash<std::string>{}(k.vertex);
                auto combine = [&h](const std::string& s) {
                    h ^= std::hash<std::string>{}(s) + 0x9e3779b9 + (h << 6)
                        + (h >> 2);
                };
                combine(k.tessControl);
                combine(k.tessEval);
                combine(k.geometry);
                combine(k.fragment);
                return h;
            }
        };

        std::unordered_map<ShaderStages, std::weak_ptr<Shader>,
                           ShaderStagesHash>
            shader_cache_;

        std::string ReadShaderFile(const fs::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                throw std::runtime_error(
                    "Shader::loadShaderFromFile: failed to open "
                    + path.string());
            }

            std::ostringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    } // namespace

    std::shared_ptr<Shader> Shader::loadShader(const ShaderStages& stages)
    {
        ShaderStages key = stages;
        if (key.vertex.empty())
            key.vertex = default_vs_shader;
        if (key.fragment.empty())
            key.fragment = default_fs_shader;

        if (auto it = shader_cache_.find(key); it != shader_cache_.end())
            if (auto locked = it->second.lock())
                return locked;

        std::shared_ptr<Shader> shader;

        if (Settings::Get<GraphicsSettings>().api == "OpenGL")
        {
            shader = GlShader::create_shader(stages);
        }
        else if (Settings::Get<GraphicsSettings>().api == "Vulkan")
        {
            Logger::LogError("Cannot create Vulkan shader",
                             " (Not implemented). ", "Defaulting to OpenGl");
            shader = GlShader::create_shader(stages);
        }
        else if (Settings::Get<GraphicsSettings>().api == "D3D11")
        {
            Logger::LogError("Cannot create DirectX 11 shader",
                             " (Not implemented). ", "Defaulting to OpenGl");
            shader = GlShader::create_shader(stages);
        }
        else
        {
            Logger::LogError(
                "Cannot create shader for an unknown graphics API.",
                " Defaulting to OpenGl");
            shader = GlShader::create_shader(stages);
        }

        shader_cache_[key] = shader;
        return shader;
    }

    std::shared_ptr<Shader> Shader::loadShader(const std::string& vShaderCode,
                                               const std::string& fShaderCode,
                                               const std::string& gShaderCode)
    {
        return loadShader(ShaderStages{ .vertex = vShaderCode,
                                        .geometry = gShaderCode,
                                        .fragment = fShaderCode });
    }

    std::shared_ptr<Shader>
    Shader::loadShaderFromFile(const ShaderFilePaths& paths)
    {
        ShaderStages stages;
        stages.vertex = ReadShaderFile(paths.vertex);
        stages.fragment = ReadShaderFile(paths.fragment);

        if (!paths.tessControl.empty())
            stages.tessControl = ReadShaderFile(paths.tessControl);
        if (!paths.tessEval.empty())
            stages.tessEval = ReadShaderFile(paths.tessEval);
        if (!paths.geometry.empty())
            stages.geometry = ReadShaderFile(paths.geometry);

        return loadShader(stages);
    }
} // namespace raphEngine::graphics
