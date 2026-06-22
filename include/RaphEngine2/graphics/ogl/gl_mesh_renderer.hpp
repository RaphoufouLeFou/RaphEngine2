#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>

#include "RaphEngine2/graphics/mesh_renderer.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "RaphEngine2/settings/graphics.hpp"
#include "gl_shader.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GLMeshRenderer : public MeshRenderer
    {
    public:
        GLMeshRenderer();
        void render(const raphEngine::objects::Mesh* mesh) const override;
        void
        render_shadows(const raphEngine::objects::Mesh* mesh) const override;

    private:
        GlShader* shader_;

        static const GlShader* current_active_shader_;
    };
} // namespace raphEngine::graphics
