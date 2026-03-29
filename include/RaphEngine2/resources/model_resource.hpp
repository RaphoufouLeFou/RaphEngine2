#pragma once

#include "RaphEngine2.hpp"
#include "export.hpp"
#include "resource.hpp"
#include "objects/mesh.hpp"

namespace raphEngine::resources
{
    class RAPHENGINE_API ModelResource : public Resource
    {
    public:
        ModelResource(const std::string& path);
    
    public: //temporary
        std::vector<std::unique_ptr<objects::Mesh>> meshes_;
    };
} // namespace raphEngine::resources
