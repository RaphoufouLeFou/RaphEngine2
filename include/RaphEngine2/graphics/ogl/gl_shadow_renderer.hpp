#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "RaphEngine2/graphics/shadow_renderer.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "graphics/ogl/gl_shader.hpp"

namespace raphEngine::graphics
{
    class GLMeshBuffers;

    class RAPHENGINE_API GLShadowRenderer : public ShadowRenderer
    {
    public:
        struct PreparedInstancedShadowBatch
        {
            GLMeshBuffers* buffers;
            unsigned int index_count;
            size_t instance_count;
        };

        GLShadowRenderer();
        void
        render_shadows(const raphEngine::objects::Mesh* mesh) const override;
        void render_shadows_instanced(
            const std::vector<const raphEngine::objects::Mesh*>& meshes)
            const override;

        static const glm::mat4& get_cascade_light_matrix(size_t layer)
        {
            return current_light_matrices_.at(layer);
        }

        static void prepare_shadows();
        static void cleanup_shadows();
        static void generate_shadows_buffer();

        static void debug_draw_lights();

        static void begin_cascade_layer(size_t layer);
        static size_t get_cascade_count();
        static PreparedInstancedShadowBatch upload_shadow_instances(
            const std::vector<const raphEngine::objects::Mesh*>& meshes);
        static void
        draw_shadow_instances(const PreparedInstancedShadowBatch& batch);

        static unsigned int depthMap;

    private:
        static void drawCascadeVolumeVisualizers(
            const std::vector<glm::mat4>& lightMatrices, Shader* shader);

        static const GlShader* current_active_shadow_shader_;
        static std::vector<glm::mat4> current_light_matrices_;
        static glm::mat4 current_cascade_light_matrix_;
    };
} // namespace raphEngine::graphics