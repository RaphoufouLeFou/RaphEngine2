#include "graphics/ogl/gl_shader.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <system_error>
#include <vector>

#include "default_shaders.hpp"
#include <RaphEngine2/logger/logger.hpp>

namespace fs = std::filesystem;

namespace raphEngine::graphics
{
    namespace
    {
        uint64_t Fnv1aHash(const std::string& data)
        {
            uint64_t hash = 14695981039346656037ull;
            for (unsigned char c : data)
            {
                hash ^= c;
                hash *= 1099511628211ull;
            }
            return hash;
        }

        fs::path GetShaderCacheDirectory()
        {
            return fs::path("shader_cache");
        }

#pragma pack(push, 1)
        struct CachedBinaryHeader
        {
            uint32_t magic;
            uint32_t version;
            GLenum format;
            uint32_t length;
        };
#pragma pack(pop)

        constexpr uint32_t kCachedBinaryMagic = 0x53484452; // "SHDR"
        constexpr uint32_t kCachedBinaryVersion = 1;

        fs::path ComputeCachePath(const ShaderStages& stages)
        {
            std::string combined = stages.vertex + "|" + stages.tessControl
                + "|" + stages.tessEval + "|" + stages.geometry + "|"
                + stages.fragment;

            const char* renderer =
                reinterpret_cast<const char*>(glGetString(GL_RENDERER));
            const char* version =
                reinterpret_cast<const char*>(glGetString(GL_VERSION));
            combined += "|";
            combined += renderer ? renderer : "";
            combined += "|";
            combined += version ? version : "";

            std::ostringstream hex;
            hex << std::hex << std::setfill('0') << std::setw(16)
                << Fnv1aHash(combined);

            return GetShaderCacheDirectory() / (hex.str() + ".bin");
        }
    } // namespace

    std::vector<GlShader*> GlShader::loadedShaders_ = std::vector<GlShader*>();

    unsigned int GlShader::CompileStage(const std::string& source,
                                        unsigned int stageType,
                                        const std::string& stageLabel)
    {
        unsigned int shader = glCreateShader(stageType);
        const char* code = source.c_str();
        glShaderSource(shader, 1, &code, NULL);
        glCompileShader(shader);
        checkCompileErrors(shader, stageLabel);
        return shader;
    }

    std::shared_ptr<GlShader>
    GlShader::create_shader(const std::string& vShaderCode,
                            const std::string& fShaderCode,
                            const std::string& gShaderCode)
    {
        return create_shader(ShaderStages{ .vertex = vShaderCode,
                                           .geometry = gShaderCode,
                                           .fragment = fShaderCode });
    }

    std::shared_ptr<GlShader>
    GlShader::create_shader(const ShaderStages& stages)
    {
        ShaderStages resolved = stages;
        if (resolved.vertex.empty())
            resolved.vertex = default_vs_shader;
        if (resolved.fragment.empty())
            resolved.fragment = default_fs_shader;

        return std::make_shared<GlShader>(resolved);
    }

    GlShader::GlShader(const std::string& vShaderCode,
                       const std::string& fShaderCode,
                       const std::string& gShaderCode)
        : GlShader(ShaderStages{ .vertex = vShaderCode,
                                 .geometry = gShaderCode,
                                 .fragment = fShaderCode })
    {}

    GlShader::GlShader(const ShaderStages& stages)
    {
        fs::path cachePath = ComputeCachePath(stages);

        if (TryLoadCachedBinary(cachePath))
        {
            GlShader::loadedShaders_.push_back(this);
            return;
        }

        Logger::LogDebug("compiling a shader");

        unsigned int vertex =
            CompileStage(stages.vertex, GL_VERTEX_SHADER, "VERTEX");
        unsigned int fragment =
            CompileStage(stages.fragment, GL_FRAGMENT_SHADER, "FRAGMENT");

        unsigned int tessControl = 0;
        if (!stages.tessControl.empty())
            tessControl = CompileStage(stages.tessControl,
                                       GL_TESS_CONTROL_SHADER, "TESS_CONTROL");

        unsigned int tessEval = 0;
        if (!stages.tessEval.empty())
            tessEval = CompileStage(stages.tessEval, GL_TESS_EVALUATION_SHADER,
                                    "TESS_EVALUATION");

        unsigned int geometry = 0;
        if (!stages.geometry.empty())
            geometry =
                CompileStage(stages.geometry, GL_GEOMETRY_SHADER, "GEOMETRY");

        id_ = glCreateProgram();

        glProgramParameteri(id_, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);

        glAttachShader(id_, vertex);
        glAttachShader(id_, fragment);
        if (tessControl != 0)
            glAttachShader(id_, tessControl);
        if (tessEval != 0)
            glAttachShader(id_, tessEval);
        if (geometry != 0)
            glAttachShader(id_, geometry);
        glLinkProgram(id_);
        checkCompileErrors(id_, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (tessControl != 0)
            glDeleteShader(tessControl);
        if (tessEval != 0)
            glDeleteShader(tessEval);
        if (geometry != 0)
            glDeleteShader(geometry);

        SaveCachedBinary(cachePath);

        GlShader::loadedShaders_.push_back(this);
    }

    bool GlShader::TryLoadCachedBinary(const fs::path& cachePath)
    {
        GLint numFormats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormats);
        if (numFormats == 0)
        {
            return false;
        }

        std::ifstream file(cachePath, std::ios::binary);
        if (!file)
        {
            return false;
        }

        CachedBinaryHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file || header.magic != kCachedBinaryMagic
            || header.version != kCachedBinaryVersion)
        {
            return false;
        }

        std::vector<char> binary(header.length);
        file.read(binary.data(), static_cast<std::streamsize>(binary.size()));
        if (!file)
        {
            return false;
        }

        unsigned int candidateId = glCreateProgram();
        glProgramBinary(candidateId, header.format, binary.data(),
                        static_cast<GLsizei>(binary.size()));

        GLint linkStatus = GL_FALSE;
        glGetProgramiv(candidateId, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE)
        {
            glDeleteProgram(candidateId);
            return false;
        }

        id_ = candidateId;
        Logger::LogDebug(
            "loaded shader from cached binary, skipped compilation");
        return true;
    }

    void GlShader::SaveCachedBinary(const fs::path& cachePath) const
    {
        GLint binaryLength = 0;
        glGetProgramiv(id_, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
        if (binaryLength <= 0)
        {
            return;
        }

        std::vector<char> binary(static_cast<size_t>(binaryLength));
        GLenum format = 0;
        GLsizei writtenLength = 0;
        glGetProgramBinary(id_, binaryLength, &writtenLength, &format,
                           binary.data());

        std::error_code ec;
        fs::create_directories(cachePath.parent_path(), ec);

        std::ofstream file(cachePath, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            return;
        }

        CachedBinaryHeader header{};
        header.magic = kCachedBinaryMagic;
        header.version = kCachedBinaryVersion;
        header.format = format;
        header.length = static_cast<uint32_t>(writtenLength);

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(binary.data(), writtenLength);
    }

    void GlShader::use() const
    {
        glUseProgram(id_);
    }

    void GlShader::checkCompileErrors(unsigned int shader,
                                      const std::string& type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                Logger::LogError("shader compilation error of type ", type,
                                 "\n", infoLog);
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                Logger::LogError("shader linking error of type ", type, "\n",
                                 infoLog);
            }
        }
    }

    int GlShader::getUniformLocation(const std::string& name) const
    {
        if (auto it = uniform_location_cache_.find(name);
            it != uniform_location_cache_.end())
            return it->second;

        GLint location = glGetUniformLocation(id_, name.c_str());
        uniform_location_cache_[name] = location;
        return location;
    }

    void GlShader::bindUniformBlock(const std::string& blockName,
                                    unsigned int bindingPoint) const
    {
        GLuint blockIndex = glGetUniformBlockIndex(id_, blockName.c_str());
        if (blockIndex == GL_INVALID_INDEX)
        {
            Logger::LogError(
                "GlShader::bindUniformBlock: no uniform block named ",
                blockName, " in this shader");
            return;
        }
        glUniformBlockBinding(id_, blockIndex, bindingPoint);
    }

    void GlShader::setValue(const std::string& name, bool value) const
    {
        glUniform1i(getUniformLocation(name), static_cast<int>(value));
    }

    void GlShader::setValue(const std::string& name, int value) const
    {
        glUniform1i(getUniformLocation(name), value);
    }

    void GlShader::setValue(const std::string& name, float value) const
    {
        glUniform1f(getUniformLocation(name), value);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::vec2& value) const
    {
        glUniform2fv(getUniformLocation(name), 1, &value.x);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::vec3& value) const
    {
        glUniform3fv(getUniformLocation(name), 1, &value.x);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::vec4& value) const
    {
        glUniform4fv(getUniformLocation(name), 1, &value[0]);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::mat2& value) const
    {
        glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::mat3& value) const
    {
        glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }

    void GlShader::setValue(const std::string& name,
                            const glm::mat4& value) const
    {
        glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }

    void GlShader::setValueArray(const std::string& name, size_t count,
                                 const int* array) const
    {
        glUniform3iv(getUniformLocation(name), count, array);
    }

    void GlShader::setValueArray(const std::string& name, size_t count,
                                 const float* array) const
    {
        glUniform3fv(getUniformLocation(name), count, array);
    }

    void GlShader::setValueArray(const std::string& name, size_t count,
                                 const glm::vec2* array) const
    {
        glUniform2fv(getUniformLocation(name), count, &array->x);
    }

    void GlShader::setValueArray(const std::string& name, size_t count,
                                 const glm::vec3* array) const
    {
        glUniform3fv(getUniformLocation(name), count, &array->x);
    }

} // namespace raphEngine::graphics
