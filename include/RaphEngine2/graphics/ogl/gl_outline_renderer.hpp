#pragma once

#include <RaphEngine2/graphics/outline_renderer.hpp>
#include <RaphEngine2/graphics/shader.hpp>
#include <memory>

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API GL_OutlineRenderer : public OutlineRenderer
    {
    public:
        void render(
            const std::vector<const objects::Mesh*>& outlined_meshes) override;

    private:
        void ensureResources(int width, int height);

        unsigned int mask_fbo_ = 0, mask_tex_ = 0, mask_depth_tex_ = 0;
        unsigned int intermediate_fbo_ = 0, intermediate_tex_ = 0;
        unsigned int dummy_vao_ = 0;
        int fbo_width_ = 0, fbo_height_ = 0;

        std::shared_ptr<Shader> mask_shader_;
        std::shared_ptr<Shader> dilate_h_shader_;
        std::shared_ptr<Shader> dilate_v_composite_shader_;
    };
} // namespace raphEngine::graphics::ogl
