#pragma once

#include <RaphEngine2/export.hpp>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <nlohmann/json.hpp>
#include "RaphEngine2/logger/logger.hpp"

namespace raphEngine::reflection
{

    struct RAPHENGINE_API FieldInfo
    {
        std::function<void(const void*, nlohmann::json&)> get;
        std::function<void(void*, const nlohmann::json&)> set;
    };

    struct RAPHENGINE_API TypeInfo
    {
        std::vector<FieldInfo> fields;
        TypeInfo* parent = nullptr;
    };

    class RAPHENGINE_API Reflection
    {
    public:
        static std::unordered_map<std::type_index, TypeInfo>& registry();

        template <typename Derived, typename T>
        static void addField(const std::string& name, T Derived::*member)
        {
            registry()[typeid(Derived)].fields.push_back(FieldInfo{
                [member, name](const void* obj, nlohmann::json& j) {
                    j[name] = static_cast<const Derived*>(obj)->*member;
                },
                [member, name](void* obj, const nlohmann::json& j) {
                    if (j.contains(name))
                        static_cast<Derived*>(obj)->*member =
                            j.at(name).get<T>();
                } });
        }

        template <typename Derived, typename Base>
        static void setParent()
        {
            registry()[typeid(Derived)].parent = &registry()[typeid(Base)];
        }
    };

    template <typename T>
    nlohmann::json toJson(const T& obj)
    {
        nlohmann::json j;
        for (const TypeInfo* t = &Reflection::registry()[typeid(obj)]; t;
             t = t->parent)
            for (const auto& field : t->fields)
                field.get(&obj, j);
        return j;
    }

    template <typename T>
    void fromJson(T& obj, const nlohmann::json& j)
    {
        for (TypeInfo* t = &Reflection::registry()[typeid(obj)]; t;
             t = t->parent)
            for (const auto& field : t->fields)
                field.set(&obj, j);
    }

    template <typename Base>
    class RAPHENGINE_API Factory
    {
    public:
        using Creator = std::function<std::unique_ptr<Base>()>;

        static std::unordered_map<std::string, Creator>& registry()
        {
            static std::unordered_map<std::string, Creator> table;
            return table;
        }

        static const std::unordered_map<std::string, Creator>& allRegistered()
        {
            return registry();
        }

        static std::unordered_map<std::type_index, std::string>& names()
        {
            static std::unordered_map<std::type_index, std::string> table;
            return table;
        }

        template <typename Derived>
        static void add(const std::string& name)
        {
            registry()[name] = [] { return std::make_unique<Derived>(); };
            names()[typeid(Derived)] = name;
        }

        static std::unique_ptr<Base> create(const std::string& name)
        {
            if (registry().contains(name))
                return registry().at(name)();
            return nullptr;
        }

        static const std::string& nameOf(const Base& obj)
        {
            return names().at(typeid(obj));
        }
    };

} // namespace raphEngine::reflection

#define REFL_EXPAND(x) x
#define REFL_FE_1(WHAT, X) WHAT(X)
#define REFL_FE_2(WHAT, X, ...)                                                \
    WHAT(X) REFL_EXPAND(REFL_FE_1(WHAT, __VA_ARGS__))
#define REFL_FE_3(WHAT, X, ...)                                                \
    WHAT(X) REFL_EXPAND(REFL_FE_2(WHAT, __VA_ARGS__))
#define REFL_FE_4(WHAT, X, ...)                                                \
    WHAT(X) REFL_EXPAND(REFL_FE_3(WHAT, __VA_ARGS__))
#define REFL_FE_5(WHAT, X, ...)                                                \
    WHAT(X) REFL_EXPAND(REFL_FE_4(WHAT, __VA_ARGS__))
#define REFL_FE_6(WHAT, X, ...)                                                \
    WHAT(X) REFL_EXPAND(REFL_FE_5(WHAT, __VA_ARGS__))
#define REFL_FE_7(WHAT, X, ...)                                                \
    WHAT(X) REFL_EXPAND(REFL_FE_6(WHAT, __VA_ARGS__))
#define REFL_FE_8(WHAT, X, ...)                                                \
    WHAT(X) REFL_EXPAND(REFL_FE_7(WHAT, __VA_ARGS__))

#define REFL_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define REFL_FOR_EACH(action, ...)                                             \
    REFL_EXPAND(REFL_GET_MACRO(__VA_ARGS__, REFL_FE_8, REFL_FE_7, REFL_FE_6,   \
                               REFL_FE_5, REFL_FE_4, REFL_FE_3, REFL_FE_2,     \
                               REFL_FE_1)(action, __VA_ARGS__))

#define REFL_REGISTER_FIELD(field)                                             \
    raphEngine::reflection::Reflection::addField(#field, &SelfType::field);

#define REFLECT_ROOT(Type)                                                     \
    static void _register_reflection();                                        \
    static const bool _reflected;

// .cpp: define both out-of-line, in the one module that owns this class.
#define REFLECT_ROOT_IMPL(Type, ...)                                           \
    void Type::_register_reflection()                                          \
    {                                                                          \
        using SelfType = Type;                                                 \
        REFL_FOR_EACH(REFL_REGISTER_FIELD, __VA_ARGS__)                        \
    }                                                                          \
    const bool Type::_reflected = (Type::_register_reflection(), true);

// For a subclass that adds new fields of its own.
#define REFLECT(Type, Base, ...)                                               \
    inline static const bool _reflected = [] {                                 \
        using SelfType = Type;                                                 \
        raphEngine::reflection::Reflection::setParent<Type, Base>();           \
        REFL_FOR_EACH(REFL_REGISTER_FIELD, __VA_ARGS__)                        \
        return true;                                                           \
    }();

#define REFLECT_EMPTY(Type, Base)                                              \
    inline static const bool _reflected = [] {                                 \
        raphEngine::reflection::Reflection::setParent<Type, Base>();           \
        return true;                                                           \
    }();

#define REFLECT_FACTORY(Type, Base, name)                                      \
    inline static const bool _factory_registered = [] {                        \
        raphEngine::reflection::Factory<Base>::template add<Type>(name);       \
        return true;                                                           \
    }();
