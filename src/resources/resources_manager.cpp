#include <memory>

#include "RaphEngine2.hpp"
#include "export.hpp"
#include "resources/resource.hpp"
#include "resources/resources_manager.hpp"

namespace raphEngine::resources
{

    std::map<std::string, std::unique_ptr<Resource>>
        ResourcesManager::loaded_resources;

} // namespace raphEngine::resources
