#include "graphics/mesh_renderer.hpp"

#include <RaphEngine2/export.hpp>
#include <memory>

#include "RaphEngine2/graphics/ogl/gl_mesh_renderer.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "settings/graphics.hpp"

namespace raphEngine::graphics
{

    std::unique_ptr<MeshRenderer> MeshRenderer::instance_ = nullptr;

    MeshRenderer* MeshRenderer::getInstance()
    {
        if (instance_ == nullptr)
        {
            // TODO select the right API
            instance_ = std::make_unique<GLMeshRenderer>();
        }

        return instance_.get();
    }
} // namespace raphEngine::graphics
