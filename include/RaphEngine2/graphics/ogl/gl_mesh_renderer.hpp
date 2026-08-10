#pragma once

#include <RaphEngine2/export.hpp>

#include "RaphEngine2/graphics/mesh_renderer.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "gl_shader.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GLMeshRenderer : public MeshRenderer
    {
    public:
        GLMeshRenderer();
        void render(const raphEngine::objects::Mesh* mesh) const override;
        void renderInstanced(
            const std::vector<const objects::Mesh*>& meshes) const override;

        void invalidate_active_shader()
        {
            current_active_shader_ = nullptr;
        }

    private:
        GlShader* shader_;

        static const GlShader* current_active_shader_;
    };
} // namespace raphEngine::graphics
