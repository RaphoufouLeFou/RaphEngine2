#pragma once

#include <RaphEngine2/graphics/texture_loader.hpp>
#include <RaphEngine2/export.hpp>
#include <string>
#include <unordered_map>

#include <filesystem>
namespace fs = std::filesystem;

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API GL_TextureLoader : public TextureLoader
    {
    public:
        unsigned int load_texture_cached(const fs::path& path,
                                         bool filter) override;
        unsigned int upload_texture_cached(const fs::path& cache_key,
                                           const RawTexture& raw,
                                           bool filter) override;

    private:
        unsigned int upload_to_gl(const RawTexture& raw, bool filter);
        std::unordered_map<fs::path, unsigned int> loaded_textures_;
    };
} // namespace raphEngine::graphics::ogl
