#include <RaphEngine2/export.hpp>
#include <RaphEngine2/logger/logger.hpp>
#include <RaphEngine2/settings/graphics.hpp>
#include <string>

namespace raphEngine
{
    const std::string& GraphicsSettings::GetKey() const
    {
        static const std::string key = "Graphics";
        return key;
    }

    nlohmann::json GraphicsSettings::ToJson() const
    {
        return { { "Api", api },
                 { "Fullscreen", fullscreen },
                 { "Resolution", resolution },
                 { "Shadow", shadow },
                 { "vSync", vSync },
                 { "AA", antiAlisaing }
                 };
    }

    void GraphicsSettings::FromJson(const nlohmann::json& j)
    {
        if (j.contains("Api"))
            j.at("Api").get_to(api);
        if (j.contains("Fullscreen"))
            j.at("Fullscreen").get_to(fullscreen);
        if (j.contains("Resolution"))
            j.at("Resolution").get_to(resolution);
        if (j.contains("Shadow"))
            j.at("Shadow").get_to(shadow);
        if (j.contains("vSync"))
            j.at("vSync").get_to(vSync);        
        if (j.contains("AA"))
            j.at("AA").get_to(antiAlisaing);
    }

    void GraphicsSettings::Reset()
    {
        *this = GraphicsSettings{};
    }

    std::pair<unsigned short, unsigned short> GraphicsSettings::getResolution()
    {
        size_t index = resolution.find('x');
        if (index == std::string::npos)
        {
            Logger::LogWarning("Failed to parse settings resolution string: ",
                               resolution);
            resolution = "1920x1080";
            return std::pair<unsigned short, unsigned short>{ 1920, 1080 };
        }

        std::string res_x = resolution.substr(0, index);
        std::string res_y = resolution.substr(index + 1);

        unsigned short int_x = std::stoi(res_x);
        unsigned short int_y = std::stoi(res_y);

        return std::pair<unsigned short, unsigned short>{ int_x, int_y };
    }

    void GraphicsSettings::setResolution(unsigned short new_x,
                                         unsigned short new_y)
    {
        std::stringstream ss{};
        ss << new_x << 'x' << new_y;
        resolution = ss.str();
    }

    int GraphicsSettings::getShadowResolution()
    {
        int res = 2048;
        if (shadow == "High")
            return res * 2;
        if (shadow == "Mid")
            return res;
        if (shadow == "Low")
            return res / 2;

        Logger::LogWarning("Cannot resolve shadow quality: ", shadow);
        shadow = "Mid";
        return getShadowResolution();
    }

} // namespace raphEngine
