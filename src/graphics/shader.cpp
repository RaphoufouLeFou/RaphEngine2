#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <graphics/ogl/gl_shader.hpp>
#include <graphics/shader.hpp>
#include <memory>
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
        struct ShaderSourceKey
        {
            std::string v, f, g;
            bool operator==(const ShaderSourceKey&) const = default;
        };
        struct ShaderSourceKeyHash
        {
            size_t operator()(const ShaderSourceKey& k) const
            {
                size_t h = std::hash<std::string>{}(k.v);
                h ^= std::hash<std::string>{}(k.f) + 0x9e3779b9 + (h << 6)
                    + (h >> 2);
                h ^= std::hash<std::string>{}(k.g) + 0x9e3779b9 + (h << 6)
                    + (h >> 2);
                return h;
            }
        };

        // weak_ptr, not permanent: the true owner of a Shader is whichever
        // MeshComponent holds it in its shared_ptr<Shader> shader_ member —
        // ObjectMesh and Mesh only hold raw, non-owning Shader* pointers.
        // Once every component using a given source pair is destroyed, this
        // entry should expire rather than leak the program forever.
        std::unordered_map<ShaderSourceKey, std::weak_ptr<Shader>,
                           ShaderSourceKeyHash>
            shader_cache_;
    } // namespace

    std::shared_ptr<Shader> Shader::loadShader(const std::string& vShaderCode,
                                               const std::string& fShaderCode,
                                               const std::string& gShaderCode)
    {
        // Resolve empty defaults up front so callers that pass nothing and
        // callers that (hypothetically) pass the literal default source
        // string both hash to the same key.
        ShaderSourceKey key{
            vShaderCode.empty() ? default_vs_shader : vShaderCode,
            fShaderCode.empty() ? default_fs_shader : fShaderCode, gShaderCode
        };

        if (auto it = shader_cache_.find(key); it != shader_cache_.end())
            if (auto locked = it->second.lock())
                return locked;

        std::shared_ptr<Shader> shader;

        if (Settings::Get<GraphicsSettings>().api == "OpenGL")
        {
            shader =
                GlShader::create_shader(vShaderCode, fShaderCode, gShaderCode);
        }
        else if (Settings::Get<GraphicsSettings>().api == "Vulkan")
        {
            Logger::LogError("Cannot create Vulkan shader",
                             " (Not implemented). ", "Defaulting to OpenGl");
            shader =
                GlShader::create_shader(vShaderCode, fShaderCode, gShaderCode);
        }
        else if (Settings::Get<GraphicsSettings>().api == "D3D11")
        {
            Logger::LogError("Cannot create DirectX 11 shader",
                             " (Not implemented). ", "Defaulting to OpenGl");
            shader =
                GlShader::create_shader(vShaderCode, fShaderCode, gShaderCode);
        }
        else
        {
            Logger::LogError(
                "Cannot create shader for an unknown grpahics API.",
                " Defaulting to OpenGl");
            shader =
                GlShader::create_shader(vShaderCode, fShaderCode, gShaderCode);
        }

        shader_cache_[key] = shader;
        return shader;
    }
} // namespace raphEngine::graphics