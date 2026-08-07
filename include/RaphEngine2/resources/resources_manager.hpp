#pragma once

#include <map>
#include <memory>

#include "RaphEngine2/RaphEngine2.hpp"
#include "RaphEngine2/export.hpp"
#include "resource.hpp"

namespace raphEngine::resources
{
    template <class T>
    concept IsResource = requires() {
        requires std::convertible_to<T, Resource>;
        requires !std::same_as<T, Resource>;
    };

    class RAPHENGINE_API ResourcesManager
    {
    public:
        template <IsResource T>
        static Resource* load_shared_resource(const std::string& path);

        template <IsResource T>
        static std::unique_ptr<Resource>
        load_unique_resource(const std::string& path);

    private:
        static std::map<std::string, std::unique_ptr<Resource>>
            loaded_resources;
    };
} // namespace raphEngine::resources

#include "resources_manager.hxx"
