#pragma once

#include <RaphEngine2/export.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace raphEngine::objects
{
    class Mesh;
}

namespace raphEngine::graphics
{
    class RAPHENGINE_API OutlineRenderer
    {
    public:
        virtual void
        render(const std::vector<const objects::Mesh*>& outlined_meshes) = 0;

        void set_outline_color(const glm::vec3& c)
        {
            outline_color_ = c;
        }
        void set_outline_width_px(int px)
        {
            outline_width_px_ = px;
        }

        static OutlineRenderer* getInstance();

    protected:
        glm::vec3 outline_color_ = glm::vec3(1.0f, 0.6f, 0.0f);
        int outline_width_px_ = 3;

    private:
        static std::unique_ptr<OutlineRenderer> instance_;
    };
} // namespace raphEngine::graphics
