#pragma once

#include <string>

#include "export.hpp"

namespace raphEngine
{
    class RAPHENGINE_API Core
    {
    public:
        static void Init(const std::string& title = "Untitled");
        static void Run();

    private:
        static void execute_starts();
        static void execute_updates();

        static void execute_components_updates();
    };
} // namespace raphEngine
