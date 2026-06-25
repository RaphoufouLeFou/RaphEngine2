#pragma once

#include <RaphEngine2/export.hpp>
#include <string>

#include "ISettingsCategory.hpp"

namespace raphEngine
{
    class RAPHENGINE_API GraphicsSettings : public ISettingsCategory
    {
    public:
        std::string api = "OpenGL";
        bool fullscreen = false;
        std::string resolution = "1920x1080";
        std::string shadow = "High";

        const std::string& GetKey() const override
        {
            static const std::string key = "Graphics";
            return key;
        }

        nlohmann::json ToJson() const override
        {
            return { { "Api", api },
                     { "Fullscreen", fullscreen },
                     { "Resolution", resolution },
                     { "Shadow", shadow } };
        }

        void FromJson(const nlohmann::json& j) override
        {
            if (j.contains("Api"))
                j.at("Api").get_to(api);
            if (j.contains("Fullscreen"))
                j.at("Fullscreen").get_to(fullscreen);
            if (j.contains("Resolution"))
                j.at("Resolution").get_to(resolution);
            if (j.contains("Shadow"))
                j.at("Shadow").get_to(shadow);
        }

        void Reset() override
        {
            *this = GraphicsSettings{};
        }
    };
} // namespace raphEngine
