#pragma once

#include <RaphEngine2/export.hpp>
#include <cassert>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>

#include "ISettingsCategory.hpp"

namespace raphEngine
{
    class RAPHENGINE_API Settings
    {
    public:
        template <typename T, typename... Args>
        static T& Register(Args&&... args);

        template <typename T>
        static T& Get();

        template <typename T>
        static bool IsRegistered();

        static bool Load(const std::string& path = "settings.json");
        static bool Save(const std::string& path = "settings.json");
        static void Reset();

        using ChangeCallback = std::function<void()>;
        static void OnChanged(ChangeCallback cb);
        static void NotifyChanged();

    private:
        Settings() = delete;

        static std::unordered_map<std::type_index,
                                  std::shared_ptr<ISettingsCategory>>
            s_byType;
        static std::unordered_map<std::string, ISettingsCategory*> s_byKey;
        static std::vector<ChangeCallback> s_callbacks;
    };
} // namespace raphEngine

#include "settings.hxx"
