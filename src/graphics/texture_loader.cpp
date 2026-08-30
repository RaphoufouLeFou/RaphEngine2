#include <RaphEngine2/graphics/texture_loader.hpp>
#include <RaphEngine2/graphics/ogl/gl_texture_loader.hpp>
#include <string>
#include <memory>

#include "settings/graphics.hpp"
#include "settings/settings.hpp"
#include <RaphEngine2/logger/logger.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace raphEngine::graphics
{
    std::unique_ptr<TextureLoader> TextureLoader::instance_ = nullptr;
    TextureLoader* TextureLoader::getInstance()
    {
        if (instance_ == nullptr)
        {
            if (Settings::Get<GraphicsSettings>().api == "OpenGL")
            {
                instance_ = std::make_unique<ogl::GL_TextureLoader>();
                return instance_.get();
            }
            if (Settings::Get<GraphicsSettings>().api == "Vulkan")
            {
                Logger::LogError("Cannot get Texture Loader for Vulkan",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<ogl::GL_TextureLoader>();
                return instance_.get();
                // TODO: for later
            }
            if (Settings::Get<GraphicsSettings>().api == "D3D11")
            {
                Logger::LogError("Cannot get Texture Loader for DirectX 11",
                                 " (Not implemented). ",
                                 "Defaulting to OpenGl");
                instance_ = std::make_unique<ogl::GL_TextureLoader>();
                return instance_.get();
                // TODO: for later
            }

            Logger::LogError(
                "Cannot get Texture Loader for an unknown grpahics API.",
                " Defaulting to OpenGl");

            instance_ = std::make_unique<ogl::GL_TextureLoader>();
        }

        return instance_.get();
    }

    TextureLoader::RawTexture
    TextureLoader::load_texture_raw(const fs::path& path)
    {
        fs::path filename = path;

        RawTexture res;

        res.data = stbi_load(filename.string().c_str(), &res.width, &res.height,
                             &res.nrChannels, 0);

        return res;
    }

    TextureLoader::RawTextureHDR
    TextureLoader::load_texture_raw_hdr(const fs::path& path)
    {
        RawTextureHDR res;
        res.data = stbi_loadf(path.string().c_str(), &res.width, &res.height,
                              &res.nrChannels, 0);
        return res;
    }

    TextureLoader::RawTexture
    TextureLoader::load_texture_raw_from_memory(const unsigned char* buffer,
                                                size_t buffer_len)
    {
        RawTexture res;
        res.data =
            stbi_load_from_memory(buffer, static_cast<int>(buffer_len),
                                  &res.width, &res.height, &res.nrChannels, 0);
        return res;
    }

    void TextureLoader::free_raw(RawTexture& raw_texture)
    {
        if (!raw_texture.data)
        {
            Logger::LogError("Can't free a null raw texture data !");
            return;
        }
        stbi_image_free(raw_texture.data);
        raw_texture.data = nullptr;
    }

    void TextureLoader::free_raw(RawTextureHDR& raw_texture)
    {
        if (!raw_texture.data)
        {
            Logger::LogError("Can't free a null raw texture data !");
            return;
        }
        stbi_image_free(raw_texture.data);
        raw_texture.data = nullptr;
    }
} // namespace raphEngine::graphics
