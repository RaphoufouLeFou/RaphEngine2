#pragma once

#include <RaphEngine2/export.hpp>
#include <string>

namespace raphEngine
{

    namespace Color
    {
        constexpr const char* Reset = "\033[0m";
        constexpr const char* Red = "\033[31m";
        constexpr const char* Green = "\033[32m";
        constexpr const char* Yellow = "\033[33m";
        constexpr const char* Blue = "\033[34m";
        constexpr const char* Magenta = "\033[35m";
        constexpr const char* Cyan = "\033[36m";
        constexpr const char* White = "\033[37m";

        constexpr const char* BgRed = "\033[41m";

        constexpr const char* IntenseRed = "\033[1;91m";
        constexpr const char* IntensePurple = "\033[1;95m";
        constexpr const char* IntenseWhite = "\033[1;97m";
        constexpr const char* IntenseBlue = "\033[1;94m";
        constexpr const char* IntenseYellow = "\033[1;93m";

    } // namespace Color

    template <typename T>
    concept Printable = requires(std::ostream& os, T value) {
        { os << value } -> std::convertible_to<std::ostream&>;
    };

    class RAPHENGINE_API Logger
    {
    public:
        enum LogLevel
        {
            CRITICAL,
            ERROR,
            WARN,
            INFO,
            DEBUG,
        };

        static void ConfigureLogger(const std::string& log_file_path,
                                    LogLevel log_level = INFO,
                                    bool std_cout = true);

        template <Printable... Args>
        static void LogInfo(Args&&... args);
        template <Printable... Args>
        static void LogDebug(Args&&... args);
        template <Printable... Args>
        static void LogWarning(Args&&... args);
        template <Printable... Args>
        static void LogError(Args&&... args);
        template <Printable... Args>
        static void LogCritical(Args&&... args);

    private:
        static std::ofstream output_stream;
        static bool std_cout_;
        static bool log_to_file_;
        static LogLevel log_level_;

        static void PrintHeaders(std::ostream& output_stream);

        template <Printable... Args>
        static void Log(const std::string& type, const char* color,
                        Args&&... args);
    };
} // namespace raphEngine

#include "logger.hxx"
