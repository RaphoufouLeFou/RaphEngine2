#pragma once

#include <string>

#include "RaphEngine2/editor/editor.hpp"
#include "export.hpp"
#include "graphics/graphic_api.hpp"

namespace raphEngine
{
    class RAPHENGINE_API Core
    {
    public:
        static int Launch(const std::string& project_file = "project.prj");

        static double GetFPS();
        static graphics::GraphicApi* getRenderer();
        static bool is_editor_mode();
        static void Quit();

    private:
        friend Editor;
        static void Init(const std::string& title = "Untitled");
        static void Run();

        static void execute_updates();
        static void execute_components_updates();

        static bool editor_mode_;
    };
} // namespace raphEngine
