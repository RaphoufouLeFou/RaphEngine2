#pragma once

#include <RaphEngine2/export.hpp>
#include <string>
#include <memory>

#include <filesystem>
namespace fs = std::filesystem;

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

        struct RawTextureHDR
        {
            float* data = nullptr;
            int width = 0, height = 0, nrChannels = 0;
        };

        virtual unsigned int load_texture_cached(const fs::path& path,
                                                 bool filter) = 0;
        virtual unsigned int upload_texture_cached(const fs::path& cache_key,
                                                   const RawTexture& raw,
                                                   bool filter) = 0;

        RawTexture load_texture_raw(const fs::path& path);
        RawTextureHDR load_texture_raw_hdr(const fs::path& path);

        RawTexture load_texture_raw_from_memory(const unsigned char* buffer,
                                                size_t buffer_len);

        void free_raw(RawTexture& raw_texture);
        void free_raw(RawTextureHDR& raw_texture);

        static TextureLoader* getInstance();

    private:
        static std::unique_ptr<TextureLoader> instance_;
    };
} // namespace raphEngine::graphics
