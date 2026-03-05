#pragma once

#include "settings/settings.hpp"

namespace raphEngine::settings
{
    std::vector<std::unique_ptr<Settings>> Settings::parse_settings(nlohmann::json parent_node)
    {
        
    }
}