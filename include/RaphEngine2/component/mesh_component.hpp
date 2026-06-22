#pragma once

#include <memory>
#include <string>

#include "RaphEngine2/export.hpp"
#include "RaphEngine2/graphics/shader.hpp"
#include "RaphEngine2/objects/lod.hpp"
#include "RaphEngine2/objects/mesh_info.hpp"
#include "RaphEngine2/renderable.hpp"
#include "component.hpp"

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
        MeshComponent(std::initializer_list<objects::MeshInfo> mesh_lods,
                      std::shared_ptr<graphics::Shader> shader = nullptr);
        MeshComponent(objects::MeshInfo mesh,
                      std::shared_ptr<graphics::Shader> shader = nullptr);
        const std::string component_name = "Render mesh";

        inline const std::string get_name() const override
        {
            return component_name;
        }

        void Start() override;
        void Update() override;
        std::unique_ptr<objects::Lod> lods_;

    protected:
        std::shared_ptr<graphics::Shader> shader_;
        std::vector<objects::MeshInfo> meshes_;

    private:
        void render() const override;
    };
} // namespace raphEngine::component
