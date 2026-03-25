#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>

#include "objects/mesh.hpp"
#include "renderer.hpp"
#include "settings/graphics.hpp"

namespace raphEngine::objects
{
    class Mesh;
}

namespace raphEngine::graphics
{

    class RAPHENGINE_API MeshRenderer : public Renderer
    {
    public:
        virtual void render() override = 0;
        virtual void render_shadows() = 0;
        static std::shared_ptr<MeshRenderer>
        createMeshRender(raphEngine::objects::Mesh* mesh);

    protected:
        MeshRenderer(objects::Mesh* mesh);
        const objects::Mesh* mesh_;
    };
} // namespace raphEngine::graphics
