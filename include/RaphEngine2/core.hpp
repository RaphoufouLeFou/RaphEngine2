#pragma once

#include <string>

#include "RaphEngine2/editor/editor.hpp"
#include "export.hpp"
#include "graphics/graphic_api.hpp"

namespace raphEngine
{
    enum class EditorMode
    {
        RUNNING,
        EDITOR,
        EDITOR_RUNNING,
    };

    class RAPHENGINE_API Core
    {
    public:
        static int Launch(const std::string& project_file = "project.prj",
                          EditorMode editor_mode = EditorMode::RUNNING);

        static double GetFPS();
        static graphics::GraphicApi* getRenderer();
        static bool is_full_editor();
        static bool is_full_running();
        static bool is_editor_mode_on();
        static bool is_running_mode_on();
        static bool is_editor_running_mode();
        static void Quit();

    private:
        friend Editor;
        static void Init(const std::string& title = "Untitled");
        static void Run();

        static void execute_updates();
        static void execute_components_updates();

        static EditorMode editor_mode_;
    };
} // namespace raphEngine
