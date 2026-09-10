#include <RaphEngine2/export.hpp>
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/settings/sound.hpp>
#include <string>
#include <sstream>

namespace raphEngine
{
    const std::string& SoundSettings::GetKey() const
    {
        static const std::string key = "Sound";
        return key;
    }

    nlohmann::json SoundSettings::ToJson() const
    {
        return { { "Master", masterVolume },
                 { "Music", musicVolume },
                 { "Effects", effectsVolume } };
    }

    void SoundSettings::FromJson(const nlohmann::json& j)
    {
        if (j.contains("Master"))
            j.at("Master").get_to(masterVolume);
        if (j.contains("Music"))
            j.at("Music").get_to(musicVolume);
        if (j.contains("Effects"))
            j.at("Effects").get_to(effectsVolume);
    }

    void SoundSettings::Reset()
    {
        *this = SoundSettings{};
    }

} // namespace raphEngine
