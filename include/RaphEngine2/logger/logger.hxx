#pragma once

#include "logger.hpp"

#include <RaphEngine2/export.hpp>
#include <ostream>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>

namespace raphEngine
{
    template <Printable... Args>
    void Logger::Log(const std::string& type, const char* color, Args&&... args)
    {
        if (std_cout_)
        {
            std::cout << "[";
            if (color)
                std::cout << color;

            std::cout << type << Color::Reset << "] " << Color::Green;
            PrintHeaders(std::cout);
            std::cout << Color::Reset << " == ";
            (std::cout << ... << std::forward<Args>(args));
            std::cout << "\n";
        }

        if (log_to_file_)
        {
            output_stream << "[" << type << "] ";
            PrintHeaders(output_stream);
            output_stream << " == ";
            (output_stream << ... << std::forward<Args>(args));
            output_stream << "\n";
        }
    }

    template <Printable... Args>
    void Logger::LogInfo(Args&&... args)
    {
        if (log_level_ < INFO)
            return;
        Log("LOG  ", Color::IntenseWhite, args...);
    }

    template <Printable... Args>
    void Logger::LogDebug(Args&&... args)
    {
        if (log_level_ < DEBUG)
            return;
        Log("DEBUG", Color::IntenseBlue, args...);
    }

    template <Printable... Args>
    void Logger::LogWarning(Args&&... args)
    {
        if (log_level_ < WARN)
            return;
        Log("WARN ", Color::IntenseYellow, args...);
    }

    template <Printable... Args>
    void Logger::LogError(Args&&... args)
    {
        if (log_level_ < ERROR)
            return;
        Log("ERROR", Color::IntenseRed, args...);
    }

} // namespace raphEngine
