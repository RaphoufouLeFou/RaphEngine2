#pragma once

#include <memory>

#include "RaphEngine2/graphics/shader.hpp"
#include "RaphEngine2/export.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "RaphEngine2/settings/graphics.hpp"

namespace raphEngine::objects
{
    class Mesh;
}

namespace raphEngine::graphics
{

    class RAPHENGINE_API MeshRenderer
    {
    public:
        virtual void render(const raphEngine::objects::Mesh* mesh) const = 0;
        virtual void
        render_shadows(const Shader* shadow_shader, const raphEngine::objects::Mesh* mesh) const = 0;
        static MeshRenderer* getInstance();

    private:
        static std::unique_ptr<MeshRenderer> instance_;
    };
} // namespace raphEngine::graphics
