#pragma once

#include <RaphEngine2/export.hpp>
#include <string>

#include "ISettingsCategory.hpp"

namespace raphEngine
{
    class RAPHENGINE_API SoundSettings : public ISettingsCategory
    {
    public:
	float masterVolume = 0.8f;
	float musicVolume = 1.0f;
	float effectsVolume = 1.0f;

        const std::string& GetKey() const override;

        nlohmann::json ToJson() const override;

        void FromJson(const nlohmann::json& j) override;
        void Reset() override;
    };
} // namespace raphEngine
