#pragma once

#include <memory>

#include "RaphEngine2.hpp"
#include "export.hpp"
#include "resource.hpp"

namespace raphEngine::resources
{
    template <class T>
    concept IsResource = requires() {
        std::convertible_to<T, Resource>;
        !std::same_as<T, Resource>;
    };

    class RAPHENGINE_API ResourcesManager
    {
    public:
        // returns nullptr and print an message on std::cerr in case of a falure
        template <IsResource T>
        static Resource* load_shared_resource(const std::string& path);

        // returns nullptr and print an message on std::cerr in case of a falure
        template <IsResource T>
        static std::unique_ptr<Resource>
        load_unique_resource(const std::string& path);

    private:
        static std::map<std::string, std::unique_ptr<Resource>>
            loaded_resources;
    };
} // namespace raphEngine::resources

#include "resources_manager.hxx"
