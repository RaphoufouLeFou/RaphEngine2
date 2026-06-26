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

        const std::string& GetKey() const override;

        nlohmann::json ToJson() const override;

        void FromJson(const nlohmann::json& j) override;
        void Reset() override;

        std::pair<unsigned short, unsigned short> getResolution();
        void setResolution(unsigned short new_x, unsigned short new_y);
    };
} // namespace raphEngine
