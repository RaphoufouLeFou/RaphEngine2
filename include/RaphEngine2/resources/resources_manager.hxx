#pragma once

#include "resources_manager.hpp"

#include "RaphEngine2.hpp"
#include "export.hpp"
#include "resources.hpp"
#include <memory>

namespace raphEngine::resources
{
    
    std::map<std::string, std::unique_ptr<Resources>> ResourcesManager::loaded_resources;

    template<IsResource T>
    Resources* ResourcesManager::load_shared_resource(const std::string& path)
    {
        try
        {
            return loaded_resources.at(path).get();
        }
        catch(const std::out_of_range& e)
        {
            std::unique_ptr<T> resource = std::make_unique<T>(path);
            Resources* res = resource.get();
            loaded_resources[path] = std::move(resource);
            return res;
        }
    }

    template<IsResource T>
    std::unique_ptr<Resources> ResourcesManager::load_unique_resource(const std::string& path)
    {
        return std::make_unique<T>(path);
    }
}
