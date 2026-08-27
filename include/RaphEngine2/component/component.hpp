#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "RaphEngine2/export.hpp"
#include "RaphEngine2/scenes/reflection.hpp"
#include <RaphEngine2/logger/logger.hpp>

namespace raphEngine::objects
{
    class GameObject;
}

namespace raphEngine::component
{
    class RAPHENGINE_API Component
    {
    public:
        virtual ~Component() = default;

        virtual void Start() = 0;
        virtual void Update() = 0;

        virtual void ImGuiPrint() = 0;

        virtual const std::string get_name() const
        {
            return "Empty component";
        };

        nlohmann::json toJson() const
        {
            return reflection::toJson(*this);
        }

        void fromJson(const nlohmann::json& j)
        {
            reflection::fromJson(*this, j);
        }

        friend void to_json(nlohmann::json& j,
                            const std::unique_ptr<Component>& c)
        {
            j = c->toJson();
            j["__component_type"] = c->get_name();
        }

        static std::unique_ptr<Component>
        parse_from_json(const nlohmann::json& j)
        {
            Logger::LogDebug("Starting comp parse");
            auto c = reflection::Factory<Component>::create(
                j.at("__component_type").get<std::string>());

            Logger::LogDebug("In comp parse");
            c->fromJson(j);
            Logger::LogDebug("Done comp parse");
            return c;
        }

        objects::GameObject* parent_object;
    };
} // namespace raphEngine::component
