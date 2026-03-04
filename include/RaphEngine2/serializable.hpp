#pragma once

#include <string>

#include "export.hpp"
#include <nlohmann/json.hpp>

namespace raphEngine
{
    class RAPHENGINE_API Serializable
    {
        virtual nlohmann::json serialize() const = 0;
        virtual bool deserialize(const nlohmann::json& input) = 0;
    };
} // namespace raphEngine
