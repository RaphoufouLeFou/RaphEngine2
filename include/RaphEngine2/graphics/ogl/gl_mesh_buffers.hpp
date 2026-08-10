#pragma once

#include <RaphEngine2/export.hpp>

#include "graphics/mesh_buffers.hpp"

namespace raphEngine::graphics
{
    class RAPHENGINE_API GLMeshBuffers : public MeshBuffers
    {
    public:
        GLMeshBuffers(raphEngine::objects::Mesh* mesh);

        unsigned int vao_;
        unsigned int vbo_;
        unsigned int ebo_;
        unsigned int instance_vbo_ = 0;

        void GenerateBuffers() override;
        void EnsureInstanceBuffer();
        void UploadInstanceData(const std::vector<glm::mat4>& matrices);

    private:
        raphEngine::objects::Mesh* mesh_;
    };

} // namespace raphEngine::graphics
