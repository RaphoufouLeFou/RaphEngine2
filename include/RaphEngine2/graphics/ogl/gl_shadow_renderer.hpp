#pragma once

#include <RaphEngine2/export.hpp>

#include "RaphEngine2/graphics/shadow_renderer.hpp"
#include "RaphEngine2/objects/mesh.hpp"

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
        static void generate_shadows_buffer();

        static void debug_draw_lights();

        static unsigned int depthMap;

    private:
        static void drawCascadeVolumeVisualizers(
            const std::vector<glm::mat4>& lightMatrices, Shader* shader);
    };
} // namespace raphEngine::graphics
