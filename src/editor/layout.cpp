#include <RaphEngine2/editor/layout.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include "imgui.h"
#include "objects/game_object.hpp"

namespace raphEngine::editor
{
    namespace
    {
        constexpr const char* kDragDropPayloadType = "RE2_HIERARCHY_GO";
        constexpr float kRootDropLeftWidthEms = 1.2f;
        constexpr float kRootDropBottomHeightEms = 1.2f;
        const ImVec4 kRootDropTargetColor = ImVec4(0.30f, 0.70f, 1.00f, 0.90f);
    } // namespace

    bool Layout::IsSelected(objects::GameObject* go)
    {
        return selected_ == go;
    }

    void Layout::SelectObject(objects::GameObject* go)
    {
        selected_ = go;
        selection_.clear();
        if (go)
            selection_.insert(go);
    }

    void Layout::UnselectAll()
    {
        SelectObject(nullptr);
    }

    int Layout::create_flags(objects::GameObject* go)
    {
        ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        tree_node_flags |= ImGuiTreeNodeFlags_NavLeftJumpsToParent;
        if (go->get_transform().get_children().size() == 0)
            tree_node_flags |=
                ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_Leaf;
        if (selection_.count(go))
            tree_node_flags |= ImGuiTreeNodeFlags_Selected;

        return tree_node_flags;
    }

    void Layout::select_all_recursive(objects::GameObject* go)
    {
        selection_.insert(go);
        for (auto* t : go->get_transform().get_children())
            select_all_recursive(t->parent_object);
    }

    void Layout::apply_range_recursive(objects::GameObject* go,
                                       objects::GameObject* first,
                                       objects::GameObject* last, bool selected,
                                       bool& in_range)
    {
        if (go == first)
            in_range = true;

        if (in_range)
        {
            if (selected)
                selection_.insert(go);
            else
                selection_.erase(go);
        }

        if (go == last)
            in_range = false;

        for (auto* t : go->get_transform().get_children())
            apply_range_recursive(t->parent_object, first, last, selected,
                                  in_range);
    }

    void Layout::apply_selection_requests(void* void_ms_io)
    {
        ImGuiMultiSelectIO* ms_io =
            static_cast<ImGuiMultiSelectIO*>(void_ms_io);
        for (const ImGuiSelectionRequest& req : ms_io->Requests)
        {
            if (req.Type == ImGuiSelectionRequestType_SetAll)
            {
                if (req.Selected)
                {
                    for (auto& t : objects::Transform::root_childs)
                        select_all_recursive(t->parent_object);
                }
                else
                {
                    selection_.clear();
                }
            }
            else if (req.Type == ImGuiSelectionRequestType_SetRange)
            {
                auto* first = reinterpret_cast<objects::GameObject*>(
                    static_cast<intptr_t>(req.RangeFirstItem));
                auto* last = reinterpret_cast<objects::GameObject*>(
                    static_cast<intptr_t>(req.RangeLastItem));
                bool in_range = false;
                for (auto& t : objects::Transform::root_childs)
                    apply_range_recursive(t->parent_object, first, last,
                                          req.Selected, in_range);
            }
        }
    }

    objects::GameObject*
    Layout::find_topmost_selected_recursive(objects::GameObject* go)
    {
        if (selection_.count(go))
            return go;

        for (auto* t : go->get_transform().get_children())
        {
            if (objects::GameObject* found =
                    find_topmost_selected_recursive(t->parent_object))
                return found;
        }
        return nullptr;
    }

    objects::GameObject* Layout::find_topmost_selected()
    {
        for (auto& t : objects::Transform::root_childs)
        {
            if (objects::GameObject* found =
                    find_topmost_selected_recursive(t->parent_object))
                return found;
        }
        return nullptr;
    }

    void Layout::reconcile_selected()
    {
        if (selected_ && selection_.count(selected_))
            return;

        selected_ = find_topmost_selected();
    }

    void Layout::reparent_to(objects::GameObject* obj,
                             objects::GameObject* new_parent)
    {
        if (!obj || obj == new_parent)
            return;

        if (!new_parent)
        {
            obj->get_transform().set_parent(nullptr);
            return;
        }

        for (objects::Transform* t = &new_parent->get_transform(); t != nullptr;
             t = t->get_parent())
        {
            if (t == &obj->get_transform())
                return;
        }

        obj->get_transform().set_parent(&new_parent->get_transform());
    }

    void Layout::queue_reparent_from_payload(objects::GameObject* new_parent)
    {
        ImGui::PushStyleColor(ImGuiCol_DragDropTarget, kRootDropTargetColor);
        const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload(kDragDropPayloadType);
        ImGui::PopStyleColor();

        if (!payload)
            return;

        objects::GameObject* dragged =
            *(objects::GameObject* const*)payload->Data;

        if (selection_.count(dragged))
        {
            for (objects::GameObject* obj : selection_)
                pending_reparents_.push_back({ obj, new_parent });
        }
        else
        {
            pending_reparents_.push_back({ dragged, new_parent });
        }
    }

    void Layout::handle_drag_drop(objects::GameObject* go)
    {
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(kDragDropPayloadType, &go,
                                      sizeof(objects::GameObject*));

            if (selection_.count(go) && selection_.size() > 1)
                ImGui::Text("%d objects", (int)selection_.size());
            else
                ImGui::Text("%s", go->get_name().c_str());

            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            queue_reparent_from_payload(go);
            ImGui::EndDragDropTarget();
        }
    }

    void Layout::handle_root_drop_left()
    {
        const float margin_width = ImGui::GetFontSize() * kRootDropLeftWidthEms;
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("##root_drop_left",
                               ImVec2(margin_width, avail.y));
        if (ImGui::BeginDragDropTarget())
        {
            queue_reparent_from_payload(nullptr);
            ImGui::EndDragDropTarget();
        }
        ImGui::SameLine();
    }

    void Layout::handle_root_drop_bottom(float width)
    {
        const float margin_height =
            ImGui::GetFontSize() * kRootDropBottomHeightEms;
        ImGui::InvisibleButton("##root_drop_bottom",
                               ImVec2(width, margin_height));
        if (ImGui::BeginDragDropTarget())
        {
            queue_reparent_from_payload(nullptr);
            ImGui::EndDragDropTarget();
        }
    }

    void Layout::go_update(objects::GameObject* go)
    {
        ImGuiTreeNodeFlags tree_node_flags = create_flags(go);

        if (pending_open_.erase(go))
            ImGui::SetNextItemOpen(true);

        ImGui::SetNextItemSelectionUserData(static_cast<ImGuiSelectionUserData>(
            reinterpret_cast<intptr_t>(go)));
        bool node_open = ImGui::TreeNodeEx(go, tree_node_flags, "%s",
                                           go->get_name().c_str());

        handle_drag_drop(go);

        if (node_open)
        {
            update_childs(go);
            ImGui::TreePop();
        }
        else if (ImGui::IsItemToggledOpen())
        {
            Logger::LogDebug("IsItemToggledOpen");
        }
    }

    void Layout::update_childs(objects::GameObject* go)
    {
        for (auto* t : go->get_transform().get_children())
        {
            go_update(t->parent_object);
        }
    }

    void Layout::Update()
    {
        ImGui::Begin("Layout", NULL, ImGuiChildFlags_FrameStyle);
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float bottom_height =
                ImGui::GetFontSize() * kRootDropBottomHeightEms;
            float spacing_y = ImGui::GetStyle().ItemSpacing.y;
            float tree_height =
                std::max(avail.y - bottom_height - spacing_y, 0.0f);

            ImGui::BeginChild("##tree_area", ImVec2(avail.x, tree_height));
            {
                handle_root_drop_left();

                ImGui::BeginGroup();

                ImGuiMultiSelectFlags ms_flags =
                    ImGuiMultiSelectFlags_ClearOnEscape
                    | ImGuiMultiSelectFlags_BoxSelect2d;
                ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(
                    ms_flags, (int)selection_.size(), -1);
                apply_selection_requests(ms_io);

                for (auto& t : objects::Transform::root_childs)
                {
                    go_update(t->parent_object);
                }

                ms_io = ImGui::EndMultiSelect();
                bool had_requests = ms_io->Requests.Size > 0;
                apply_selection_requests(ms_io);

                if (had_requests)
                {
                    if (ms_io->NavIdSelected)
                        selected_ = reinterpret_cast<objects::GameObject*>(
                            static_cast<intptr_t>(ms_io->NavIdItem));

                    reconcile_selected();
                }

                ImGui::EndGroup();
            }
            ImGui::EndChild();

            handle_root_drop_bottom(avail.x);
        }
        ImGui::End();

        for (const PendingReparent& p : pending_reparents_)
        {
            reparent_to(p.object, p.new_parent);
            if (p.new_parent)
                pending_open_.insert(p.new_parent);
        }
        pending_reparents_.clear();
    }

} // namespace raphEngine::editor
