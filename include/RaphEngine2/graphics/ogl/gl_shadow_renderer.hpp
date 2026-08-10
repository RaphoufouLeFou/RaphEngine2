#pragma once

#include <RaphEngine2/export.hpp>

#include "RaphEngine2/graphics/shadow_renderer.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "graphics/ogl/gl_shader.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GLShadowRenderer : public ShadowRenderer
    {
    public:
        GLShadowRenderer();
        void
        render_shadows(const raphEngine::objects::Mesh* mesh) const override;
        void render_shadows_instanced(
            const std::vector<const raphEngine::objects::Mesh*>& meshes)
            const override;

        static void prepare_shadows();
        static void cleanup_shadows();
        static void generate_shadows_buffer();

        static void debug_draw_lights();

        static unsigned int depthMap;

    private:
        static void drawCascadeVolumeVisualizers(
            const std::vector<glm::mat4>& lightMatrices, Shader* shader);

        static const GlShader* current_active_shadow_shader_;
    };
} // namespace raphEngine::graphics
