#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>

#include "RaphEngine2/graphics/mesh_renderer.hpp"
#include "RaphEngine2/settings/graphics.hpp"
#include "gl_shader.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GLMeshRenderer : public MeshRenderer
    {
    public:
        GLMeshRenderer();
        void render(raphEngine::objects::Mesh* mesh) override;
        void render_shadows(raphEngine::objects::Mesh* mesh) override;

    private:
        GlShader* shader_;
    };
} // namespace raphEngine::graphics
