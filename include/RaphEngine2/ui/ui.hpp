#pragma once
#include <RaphEngine2/export.hpp>
#include <RaphEngine2/ui/ui_document.hpp>
#include <string>

namespace raphEngine
{
    class RAPHENGINE_API UI
    {
    public:
        static void LoadFont(const std::string& path);
        static UIDocument LoadDocument(const std::string& path);
    };
} // namespace raphEngine