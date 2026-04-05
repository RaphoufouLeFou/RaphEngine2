#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>

#include "graphics/mesh_renderer.hpp"
#include "settings/graphics.hpp"
#include "gl_shader.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GLMeshRenderer : public MeshRenderer
    {
    public:
        GLMeshRenderer(objects::Mesh* mesh);
        void render() override;
        void render_shadows() override;

    private:
        void GenerateBuffers();

        unsigned int vao_;
        unsigned int vbo_;
        unsigned int ebo_;

        GlShader* shader_;
    };
} // namespace raphEngine::graphics
