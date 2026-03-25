#pragma once

#include "RaphEngine2.hpp"
#include "export.hpp"
#include "resources.hpp"
#include <memory>

namespace raphEngine::resources
{
    template<class T>
    concept IsResource = requires()
    {
        std::convertible_to<T, Resources>;
        !std::same_as<T, Resources>;
    };

    class RAPHENGINE_API ResourcesManager
    {
    public:
        // returns nullptr and print an message on std::cerr in case of a falure
        template<IsResource T>
        static Resources* load_shared_resource(const std::string& path);

        // returns nullptr and print an message on std::cerr in case of a falure
        template<IsResource T>
        static std::unique_ptr<Resources> load_unique_resource(const std::string& path);

    private:
        static std::map<std::string, std::unique_ptr<Resources>> loaded_resources;
    };
}

#include "resources_manager.hxx"
