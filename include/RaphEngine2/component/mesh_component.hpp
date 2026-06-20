#pragma once

#include "RaphEngine2/export.hpp"
#include <memory>
#include <string>

#include "component.hpp"
#include "RaphEngine2/objects/mesh_info.hpp"
#include "RaphEngine2/renderable.hpp"
#include "RaphEngine2/objects/lod.hpp"
#include "RaphEngine2/graphics/shader.hpp"

namespace raphEngine::objects
{
    class Lod;
}

namespace raphEngine::component
{

    class RAPHENGINE_API MeshComponent
        : public Component
        , public Renderable
    {
    public:
        MeshComponent(std::initializer_list<objects::MeshInfo> mesh_lods, std::shared_ptr<graphics::Shader> shader = nullptr);
        MeshComponent(objects::MeshInfo mesh, std::shared_ptr<graphics::Shader> shader = nullptr);
        const std::string component_name = "Render mesh";

        inline const std::string get_name() const override
        {
            return component_name;
        }

        void Start() override;
        void Update() override;

    protected:
        std::unique_ptr<objects::Lod> lods_;
        std::shared_ptr<graphics::Shader> shader_;

    private:
        void render() override;
    };
} // namespace raphEngine::component
