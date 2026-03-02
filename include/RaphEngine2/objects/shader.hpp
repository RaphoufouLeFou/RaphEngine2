#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

#include <RaphEngine2/export.hpp>

namespace raphEngine::objects
{
class RAPHENGINE_API Shader
{
public:
    unsigned int ID;
    Shader(const char* vShaderCode, const char* fShaderCode, const char* gShaderCode = nullptr);
    static void Prepare();
    void use();
    void setBool(const char* name, bool value) const;
    void setInt(const char* name, int value) const;
    void setIntArray(const char* name, int count, int *value) const;
    void setFloat(const char* name, float value) const;
    void setFloatArray(const char* name, int count, float *value) const;
    void setVec2(const char* name, const glm::vec2 value) const;
    void setVec2(const char* name, float x, float y) const;
    void setVec2Array(const char* name, int count, const glm::vec2* value) const;
    void setVec3(const char* name, const glm::vec3 value) const;
    void setVec3Array(const char* name, int count, const glm::vec3 *value) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec4(const char* name, const glm::vec4& value) const;
    void setVec4(const char* name, float x, float y, float z, float w);
    void setMat2(const char* name, const glm::mat2& mat) const;
    void setMat3(const char* name, const glm::mat3& mat) const;
    void setMat4(const char* name, const glm::mat4& mat) const;
    void setModel(const glm::mat4& mat, int index) const;
    static std::vector<Shader*> LoadedShaders;

private:
    void checkCompileErrors(unsigned int shader, std::string type);
};
}
