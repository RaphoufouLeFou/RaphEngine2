#pragma once

#include <RaphEngine2/export.hpp>
#include <memory>
#include <string>

#include "component.hpp"
#include "objects/lod.hpp"
#include "objects/mesh_info.hpp"
#include "renderable.hpp"

namespace raphEngine::component
{
    class RAPHENGINE_API MeshComponent
        : public Component
        , public Renderable
    {
    public:
        MeshComponent(std::initializer_list<objects::MeshInfo> mesh_lods);
        const std::string component_name = "Render mesh";

        inline const std::string get_name() const override
        {
            return component_name;
        }

        void Start() override;
        void Update() override;

    protected:
        std::unique_ptr<objects::Lod> lods_;

    private:
        void render() override;
    };
} // namespace raphEngine::component
