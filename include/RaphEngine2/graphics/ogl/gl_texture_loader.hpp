#pragma once

#include <RaphEngine2/graphics/texture_loader.hpp>
#include <RaphEngine2/export.hpp>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API GL_TextureLoader : public TextureLoader
    {
    public:
        unsigned int load_texture_cached(const std::string& path, bool filter) override;

    private:
        std::unordered_map<std::string, unsigned int> loaded_textures_;
    };
}