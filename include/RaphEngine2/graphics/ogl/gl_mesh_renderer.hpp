#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>
#include "graphics/mesh_renderer.hpp"
#include "settings/graphics.hpp"

namespace raphEngine::graphics {
    class RAPHENGINE_API GLMeshRenderer : public MeshRenderer
    {
    public:
        GLMeshRenderer(objects::Mesh * mesh)
        {
            // TODO: implement
            (void) mesh;
        }

        inline void render() override
        {
            // TODO: implement
        }
    };
}
