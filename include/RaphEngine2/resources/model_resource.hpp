#pragma once

#include "RaphEngine2/RaphEngine2.hpp"
#include "RaphEngine2/export.hpp"
#include "RaphEngine2/objects/mesh.hpp"
#include "resource.hpp"

namespace raphEngine::resources
{
    class RAPHENGINE_API ModelResource : public Resource
    {
    public:
        ModelResource(const std::string& path);

    public: // temporary
        std::vector<std::unique_ptr<objects::Mesh>> meshes_;
    };
} // namespace raphEngine::resources
