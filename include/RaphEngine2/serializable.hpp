#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "export.hpp"

namespace raphEngine
{
    class RAPHENGINE_API Serializable
    {
        virtual nlohmann::json serialize() const = 0;
        virtual bool deserialize(const nlohmann::json& input) = 0;
    };
} // namespace raphEngine
