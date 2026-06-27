#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>

#include "RaphEngine2/graphics/shadow_renderer.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "RaphEngine2/settings/graphics.hpp"
#include "gl_shader.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GLShadowRenderer : public ShadowRenderer
    {
    public:
        GLShadowRenderer();
        void
        render_shadows(const raphEngine::objects::Mesh* mesh) const override;

        static void prepare_shadows();
        static void cleanup_shadows();

    private:
        static void generate_shadows_buffer();
    };
} // namespace raphEngine::graphics
