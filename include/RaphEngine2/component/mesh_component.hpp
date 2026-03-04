#pragma once

#include <memory>
#include <RaphEngine2/export.hpp>
#include <string>

#include "renderable.hpp"
#include "objects/lod.hpp"

#include "component.hpp"

namespace raphEngine::component
{
    class RAPHENGINE_API MeshComponent : public Component,  public Renderable
    {
    public:
        MeshComponent() : Renderable() {}
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
}
