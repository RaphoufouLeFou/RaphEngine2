#pragma once

#include "settings.hpp"

namespace raphEngine
{
    template <typename T, typename... Args>
    T& Settings::Register(Args&&... args)
    {
        static_assert(std::is_base_of_v<ISettingsCategory, T>,
                      "T must derive from ISettingsCategory");

        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        T* raw = ptr.get();

        s_byType[std::type_index(typeid(T))] = ptr;
        s_byKey[raw->GetKey()] = raw;
        return *raw;
    }

    template <typename T>
    T& Settings::Get()
    {
        auto it = s_byType.find(std::type_index(typeid(T)));
        assert(
            it != s_byType.end()
            && "Settings category not registered — call Register<T>() first");
        return static_cast<T&>(*it->second);
    }

    template <typename T>
    bool Settings::IsRegistered()
    {
        return s_byType.count(std::type_index(typeid(T))) > 0;
    }
} // namespace raphEngine
