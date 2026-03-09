#pragma once

#include <RaphEngine2/export.hpp>
#include "generic_settings.hpp"

namespace raphEngine::settings
{
    template<typename T, size_t size>
    void EnumSetting<T, size>::add_to_json(nlohmann::json& parent_node)const
    {
        parent_node[get_pretty_name()] = names_[static_cast<int>(value)];
    }

    template<typename T, size_t size>
    void EnumSetting<T, size>::from_json(const nlohmann::json& parent_node)
    {
        std::string n = parent_node.at(get_pretty_name()).template get<std::string>();
        for (size_t i = 0; i < size; i++) {
            if(names_[i] == n)
            {
                value = static_cast<T>(i);
            }
        }
    }
}