#pragma once

#include <RaphEngine2/export.hpp>
#include "settings.hpp"
#include "settings/save_settings.hpp"
#include "graphics/resolution.hpp"
#include <cstddef>
#include <nlohmann/json.hpp>
#include <string>

namespace raphEngine::settings {

    template<typename T>
    class RAPHENGINE_API EnumSetting : public Settings
    {

    public:

        ~EnumSetting() = default;

        virtual inline const std::string get_pretty_name() const override = 0;

        void add_to_json(nlohmann::json& parent_node) override;
        void from_json(nlohmann::json& parent_node) override;

        virtual inline const std::string get_name() const = 0;
        virtual const std::string& get_value_string_at(int index) const = 0;
        virtual size_t get_enum_size() const = 0;

        T value;
    };

    enum class Api;

    class RAPHENGINE_API ApiSetting : public EnumSetting<Api>
    {

    public:
        enum class Api{
    
            OPENGL = 0,
            VULAKAN,
            DX11,
        };

        constexpr static std::string names[] = 
        {
            "openGL",
            "Vulkan",
            "DX11",
        };

        static inline const std::string get_name_static() 
        {
            return name;
        }

        inline const std::string get_name() const override
        {
            return name;
        }

        const inline std::string get_pretty_name() const override
        {
            return "Graphic API";
        }

        const std::string& get_value_string_at(int index) const override
        {
            return names[index];
        }

        virtual size_t get_enum_size() const override
        {
            return names->size();
        }
        
        private:
            static std::string name;
            static bool registered;
    };

    enum class Quality;
    class RAPHENGINE_API QualitySetting : public EnumSetting<Quality>
    {
    public:
        ~QualitySetting() = default;
        enum class Quality
        {
            HIGH,
            MEDIUM,
            LOW,
        };
    
        constexpr static std::string names[] = 
        {
            "High",
            "Medium",
            "Low",
        };

        const std::string& get_value_string_at(int index) const override
        {
            return names[index];
        }

        virtual size_t get_enum_size() const override
        {
            return names->size();
        }
        
    };

    class RAPHENGINE_API BooleanSetting : public Settings
    {
    public:
        ~BooleanSetting() = default;

        virtual inline const std::string get_pretty_name() const override = 0;

        void add_to_json(nlohmann::json& parent_node) override;
        void from_json(nlohmann::json& parent_node) override;

        virtual inline const std::string get_name() const = 0;
        
        bool value;
    };

    class RAPHENGINE_API IntegerSetting : public Settings
    {
    public:
        ~IntegerSetting() = default;

        virtual inline const std::string get_pretty_name() const override = 0;

        void add_to_json(nlohmann::json& parent_node) override;
        void from_json(nlohmann::json& parent_node) override;

        virtual inline const std::string get_name() const = 0;
        
        long value;
    };

    class RAPHENGINE_API StringSetting : public Settings
    {
    public:
        ~StringSetting() = default;

        virtual inline const std::string get_pretty_name() const override = 0;

        void add_to_json(nlohmann::json& parent_node) override;
        void from_json(nlohmann::json& parent_node) override;

        virtual inline const std::string get_name() const = 0;
        
        std::string value;
    };

    class RAPHENGINE_API FullscreenSetting : public BooleanSetting
    {
        static inline const std::string get_name_static() 
        {
            return name;
        }

        inline const std::string get_name() const override
        {
            return name;
        }

        const inline std::string get_pretty_name() const override
        {
            return "Fullscreen mode";
        }
        
        private:
            static std::string name;
            static bool registered;
    };

    
    class RAPHENGINE_API ShadowSetting : public QualitySetting
    {
        static inline const std::string get_name_static() 
        {
            return name;
        }

        inline const std::string get_name() const override
        {
            return name;
        }

        const inline std::string get_pretty_name() const override
        {
            return "Shadow quality";
        }
        
        private:
            static std::string name;
            static bool registered;
    };

    class RAPHENGINE_API ResolutionSetting : public QualitySetting
    {

        enum class Quality
        {
            R3840X2160,
            R2560X1440,
            R1920X1080,
            R1280X720,
        };
    
        constexpr static std::string names[] = 
        {
            "3840 x 2160",
            "2560 x 1440",
            "1920 x 1080",
            "1280 x 720",
        };

        static inline const std::string get_name_static() 
        {
            return name;
        }

        inline const std::string get_name() const override
        {
            return name;
        }

        const inline std::string get_pretty_name() const override
        {
            return "Resolution";
        }

        const std::string& get_value_string_at(int index) const override
        {
            return names[index];
        }

        virtual size_t get_enum_size() const override
        {
            return names->size();
        }
        
        private:
            static std::string name;
            static bool registered;
    };


}
