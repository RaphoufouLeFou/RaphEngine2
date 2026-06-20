#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>

#include "RaphEngine2/objects/mesh.hpp"
#include "RaphEngine2/settings/graphics.hpp"

namespace raphEngine::objects
{
    class Mesh;
}

namespace raphEngine::graphics
{

    class RAPHENGINE_API MeshBuffers
    {
    public:
            
        // should generate the needed buffers
        static std::unique_ptr<MeshBuffers> getMeshBuffer(raphEngine::objects::Mesh* mesh);
    };

} // namespace raphEngine::graphics
