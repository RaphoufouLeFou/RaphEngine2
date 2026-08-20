#pragma once

#include <RaphEngine2/graphics/skybox.hpp>
#include <RaphEngine2/export.hpp>
#include <RaphEngine2/graphics/shader.hpp>
#include <memory>
#include <string>

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API GL_Skybox : public Skybox
    {
    public:
        void set_hdr(const std::string& hdr) override;
        void render() override;

        void set_exposure(float e)
        {
            exposure_ = e;
        }

    private:
        unsigned int cube_map_buffer_ = 0;
        unsigned int skybox_vao_ = 0;
        unsigned int skybox_vbo_ = 0;
        std::shared_ptr<Shader> skybox_shader_;
        float exposure_ = 0.5f;
    };
} // namespace raphEngine::graphics::ogl
