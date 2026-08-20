#pragma once

#include <RaphEngine2/graphics/skybox.hpp>
#include <RaphEngine2/export.hpp>
#include <vector>
#include <string>
#include <memory>

namespace raphEngine::graphics::ogl
{
    class RAPHENGINE_API GL_Skybox : public Skybox
    {
    public:
        void set_faces(const std::vector<std::string>& faces) override;
        void render() override;

    private:
        unsigned int cube_map_buffer_;
    };
}