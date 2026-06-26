#include <RaphEngine2/logger/logger.hpp>

#include <RaphEngine2/export.hpp>
#include <ostream>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>

namespace raphEngine
{
    void Logger::PrintHeaders(std::ostream& output_stream)
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::floor<std::chrono::seconds>(now);

        output_stream << std::format("{:%H:%M:%S}", time);
    }

    std::ofstream Logger::output_stream;
    bool Logger::std_cout_ = true;
    bool Logger::log_to_file_ = false;
    Logger::LogLevel Logger::log_level_;

    void Logger::ConfigureLogger(const std::string& log_file_path,
                                 LogLevel log_level, bool std_cout)
    {
        std_cout_ = std_cout;
        log_level_ = log_level;

        if (log_file_path.empty())
        {
            log_to_file_ = false;
            return;
        }

        log_to_file_ = true;
        output_stream.open(log_file_path);

        if (!output_stream.is_open())
        {
            log_to_file_ = false;
            return;
        }
    }
} // namespace raphEngine
