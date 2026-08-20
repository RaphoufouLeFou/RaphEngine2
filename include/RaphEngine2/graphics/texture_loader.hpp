#pragma once

#include <RaphEngine2/export.hpp>
#include <vector>
#include <string>
#include <memory>

namespace raphEngine::graphics
{
    class RAPHENGINE_API TextureLoader
    {
    public:
        struct RawTexture
        {
            unsigned char* data;
            int width, height, nrChannels;
        };

        virtual unsigned int load_texture_cached(const std::string& path, bool filter) = 0;

        RawTexture load_texture_raw(const std::string& path);
        void free_raw(RawTexture& raw_texture);

        static TextureLoader* getInstance();
    private:
        static std::unique_ptr<TextureLoader> instance_;
    };
}