#include "RaphEngine2/core.hpp"

#include "RaphEngine2/time_utils.hpp"
#include "graphics/ogl/opengl.hpp"
#include "graphics/debug.hpp"
#include "logger/logger.hpp"
#include "objects/game_object.hpp"
#include "objects/transform.hpp"
#include "settings/graphics.hpp"
#include "settings/settings.hpp"

#ifdef EDITOR_BUILD
#    include "imgui.h"
#    include "imgui_internal.h"
#endif

namespace raphEngine
{
    graphics::ogl::OpenGL renderer{};

    void Core::Init(const std::string& title)
    {
        Logger::ConfigureLogger("log.txt", Logger::DEBUG);
        Logger::LogDebug("Hello world from RaphEngine2!");

        Settings::Register<GraphicsSettings>();
        Settings::Load("settings.json");

#ifdef EDITOR_BUILD
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::StyleColorsDark();
#endif

        renderer.Init(title);
        graphics::Debug::getInstance()->Init();
    }

    void Core::Run()
    {
        Logger::LogDebug("running now from RaphEngine2!");

        while (1)
        {
            double start = Time::GetTime();
            renderer.StartFrame();

#ifdef EDITOR_BUILD
            ImGui::NewFrame();

            ImGuiID dockspace_id = ImGui::GetID("My Dockspace");
            ImGuiViewport* viewport = ImGui::GetMainViewport();

            // Create settings
            if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
            {
                ImGui::DockBuilderAddNode(dockspace_id,
                                          ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
                ImGuiID dock_id_left = 0;
                ImGuiID dock_id_right = 0;
                ImGuiID dock_id_down = 0;
                ImGuiID dock_id_main = dockspace_id;
                ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Down, 0.20f,
                                            &dock_id_down, &dock_id_main);

                ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Left, 0.20f,
                                            &dock_id_left, &dock_id_main);

                ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Right, 0.20f,
                                            &dock_id_right, &dock_id_main);
                ImGui::DockBuilderDockWindow("Layout", dock_id_left);

                ImGui::DockBuilderDockWindow("Down", dock_id_down);
                /*
                for (auto& go :
                objects::GameObject::spawned_game_objects_)
                {
                    std::string display_name =
                        go->get_name() + "###" + std::to_string(go->id_);
                    ImGui::DockBuilderDockWindow(display_name.c_str(),
                                                 dock_id_right);
                }
                */
                ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
                ImGui::DockBuilderDockWindow("Viewport", dock_id_main);

                ImGui::DockBuilderFinish(dockspace_id);
            }

            ImGui::DockSpaceOverViewport(
                dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

            ImGui::Begin("Down");
            static auto t = Time::GetTime();
            static double fps = 60;
            static double fps_avr = 0;
            static int avr_count = 0;

            fps_avr += 1.0f / Time::deltaTime;
            avr_count++;

            if (Time::GetTime() - t > 1000)
            {
                fps = fps_avr / avr_count;
                fps_avr = 0;
                avr_count = 0;
                t = Time::GetTime();
            }
            ImGui::Text("FPS: %f", fps);
            ImGui::End();

            ImGui::BeginMainMenuBar();
            if (ImGui::BeginMenu("File"))
            {
                ImGui::Text("File menu");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::Text("Edit menu");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help"))
            {
                ImGui::Text("Help menu");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();

            ImGui::Begin("Viewport");
            graphics::GraphicApi::viewport_focused = ImGui::IsWindowFocused();

            ImVec2 viewportScreenPos = ImGui::GetCursorScreenPos();
            graphics::GraphicApi::viewport_pos_x = viewportScreenPos.x;
            graphics::GraphicApi::viewport_pos_y = viewportScreenPos.y;

            ImVec2 avail = ImGui::GetContentRegionAvail();
            renderer.ResizeViewportFramebuffer((int)avail.x, (int)avail.y);
            ImGui::Image(renderer.GetViewportTexture(), avail, ImVec2(0, 1),
                         ImVec2(1, 0));
            ImGui::End();

            ImGuiIO& io = ImGui::GetIO();
            if (graphics::GraphicApi::viewport_focused)
                io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            else
                io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
#endif

            execute_updates();
            execute_components_updates();

            graphics::Debug::getInstance()->DrawLine(glm::vec3(0, 0, 0),
                                                     glm::vec3(100, 2, 3));
            renderer.Render();

#ifdef EDITOR_BUILD
            ImGui::Render();
#endif
            bool still_alive = renderer.Refresh();

            if (!still_alive)
            {
                break;
            }
            Time::deltaTime = (Time::GetTime() - start) / 1000.0;
        }

        Settings::Save("settings.json");
        Logger::LogDebug("exiting now!");

#ifdef EDITOR_BUILD
        ImGui::DestroyContext();
#endif
    }

    void Core::execute_updates()
    {
#ifdef EDITOR_BUILD
        ImGui::Begin("Layout");
        for (auto& t : objects::Transform::root_childs)
        {
            t->parent_object->ImGui_layout();
        }
        ImGui::End();

        ImGui::Begin("Inspector");

#endif
        for (auto& go : objects::GameObject::spawned_game_objects_)
        {
            go->pre_update();
#ifdef EDITOR_BUILD
            go->ImGui_update();
#endif
            go->Update();
        }
#ifdef EDITOR_BUILD
        ImGui::End();
#endif
    }

    void Core::execute_components_updates()
    {
        for (auto& go : objects::GameObject::spawned_game_objects_)
        {
            go->update_components();
        }
    }
} // namespace raphEngine
