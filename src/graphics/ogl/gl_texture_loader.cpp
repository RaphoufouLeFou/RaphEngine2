#include <RaphEngine2/graphics/ogl/gl_texture_loader.hpp>
#include <RaphEngine2/export.hpp>
#include "logger/logger.hpp"
#include <vector>
#include <string>
#include <memory>

#include <GL/glew.h>
#include <GL/gl.h>

namespace raphEngine::graphics::ogl
{
    unsigned int GL_TextureLoader::load_texture_cached(const std::string& path, bool filter)
    {

        if(loaded_textures_.contains(path))
        {
            return loaded_textures_[path];
        }

        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        RawTexture tex = load_texture_raw(path);
        if (tex.data)
        {
            GLenum format = GL_RGBA;
            if (tex.nrChannels == 1)
                format = GL_RED;
            else if (tex.nrChannels == 3)
                format = GL_RGB;
            else if (tex.nrChannels == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, format, tex.width, tex.height, 0, format,
                            GL_UNSIGNED_BYTE, tex.data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            if (filter)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR_MIPMAP_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_NEAREST);
            }

            glGenerateMipmap(GL_TEXTURE_2D);
            Logger::LogDebug("Texture loaded at path: ", path);
            
        }
        else
        {
            Logger::LogWarning("Texture failed to load at path: ",
                                path);
        }

        free_raw(tex);
        loaded_textures_[path] = textureID;
        return textureID;
    }

}