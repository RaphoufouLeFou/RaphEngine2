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

        unsigned int get_environment_map() const
        {
            return cube_map_buffer_;
        }
        unsigned int get_irradiance_map() const
        {
            return irradiance_map_;
        }

        void set_ambient_intensity(float a)
        {
            ambient_intensity_ = a;
        }
        float get_ambient_intensity() const
        {
            return ambient_intensity_;
        }

        void set_reflection_exposure(float e)
        {
            reflection_exposure_ = e;
        }
        float get_reflection_exposure() const
        {
            return reflection_exposure_;
        }
        bool is_loaded() const
        {
            return cube_map_buffer_ != 0 && irradiance_map_ != 0;
        }

        static constexpr float kMaxReflectionLod = 10.0f;

    private:
        unsigned int cube_map_buffer_ = 0;
        unsigned int irradiance_map_ = 0;
        unsigned int skybox_vao_ = 0;
        unsigned int skybox_vbo_ = 0;
        std::shared_ptr<Shader> skybox_shader_;
        float exposure_ = 0.1f;
        float ambient_intensity_ = 1.f;
        float reflection_exposure_ = exposure_;
    };
} // namespace raphEngine::graphics::ogl
