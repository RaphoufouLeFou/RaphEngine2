#pragma once

#include <memory>

#include "RaphEngine2.hpp"
#include "export.hpp"
#include "resource.hpp"
#include "resources_manager.hpp"

namespace raphEngine::resources
{

    template <IsResource T>
    Resource* ResourcesManager::load_shared_resource(const std::string& path)
    {
        try
        {
            return loaded_resources.at(path).get();
        }
        catch (const std::out_of_range& e)
        {
            std::unique_ptr<T> resource = std::make_unique<T>(path);
            Resource* res = resource.get();
            loaded_resources[path] = std::move(resource);
            return res;
        }
    }

    template <IsResource T>
    std::unique_ptr<Resource>
    ResourcesManager::load_unique_resource(const std::string& path)
    {
        return std::make_unique<T>(path);
    }
} // namespace raphEngine::resources
