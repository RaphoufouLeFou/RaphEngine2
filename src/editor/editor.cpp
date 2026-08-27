#include <RaphEngine2/editor/editor.hpp>
#include <memory>

#include "editor/editor_camera.hpp"
#include "graphics/graphic_api.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "scenes/scene_manager.hpp"

namespace raphEngine
{
    std::unique_ptr<EditorCamera> editor_camera;
    void Editor::Init()
    {
        editor_camera = std::make_unique<EditorCamera>();
    }

    void Editor::Update()
    {
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

            ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Left, 0.12f,
                                        &dock_id_left, &dock_id_main);

            ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Right, 0.20f,
                                        &dock_id_right, &dock_id_main);

            ImGui::DockBuilderDockWindow("Layout", dock_id_left);
            ImGui::DockBuilderDockWindow("Objects", dock_id_down);
            ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
            ImGui::DockBuilderDockWindow("Viewport", dock_id_main);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpaceOverViewport(dockspace_id, viewport,
                                     ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::Begin("Objects");
        ImGui::SeparatorText("Spawn GameObjects");

        auto objs = reflection::Factory<objects::GameObject>::allRegistered();

        for (auto [name, creator] : objs)
        {
            if (ImGui::Button(name.c_str()))
            {
                Logger::LogDebug("creating ", name);

                if (SceneManager::get_active_scene())
                    SceneManager::get_active_scene()->add_gameobject(creator());
            }
        }

        ImGui::End();

        ImGui::Begin("Prefabs");
        ImGui::SeparatorText("Spawn Prefab");

        for (auto [name, creator] : objs)
        {
            if (ImGui::Button(name.c_str()))
            {
                Logger::LogDebug("creating ", name);

                if (SceneManager::get_active_scene())
                    SceneManager::get_active_scene()->add_gameobject(creator());
            }
        }

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
        ImGui::Text("FPS: %f", Core::GetFPS());
        graphics::GraphicApi::viewport_focused = ImGui::IsWindowFocused();

        ImVec2 viewportScreenPos = ImGui::GetCursorScreenPos();
        graphics::GraphicApi::viewport_pos_x = viewportScreenPos.x;
        graphics::GraphicApi::viewport_pos_y = viewportScreenPos.y;

        ImVec2 avail = ImGui::GetContentRegionAvail();
        Core::getRenderer()->ResizeViewportFramebuffer(avail.x, avail.y);
        ImGui::Image(Core::getRenderer()->GetViewportTexture(), avail,
                     ImVec2(0, 1), ImVec2(1, 0));

        ImGui::End();

        ImGuiIO& io = ImGui::GetIO();
        if (graphics::GraphicApi::viewport_focused)
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        else
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;

        SceneManager::Imgui_update();
    }

} // namespace raphEngine
