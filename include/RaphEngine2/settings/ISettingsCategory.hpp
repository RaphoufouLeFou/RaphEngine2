#pragma once

#include <RaphEngine2/export.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace raphEngine
{
    class RAPHENGINE_API ISettingsCategory
    {
    public:
        virtual ~ISettingsCategory() = default;

        virtual const std::string& GetKey() const = 0;
        virtual nlohmann::json ToJson() const = 0;
        virtual void FromJson(const nlohmann::json& j) = 0;
        virtual void Reset() = 0;
    };
} // namespace raphEngine
