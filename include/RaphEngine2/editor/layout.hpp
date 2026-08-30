#pragma once

#include "objects/game_object.hpp"
#include <unordered_set>
#include <vector>

namespace raphEngine::editor
{
    class RAPHENGINE_API Layout
    {
    public:
        static void Update();
        static void SelectObject(objects::GameObject* go);
        static void UnselectAll();
        static bool IsSelected(objects::GameObject* go);

    private:
        struct PendingReparent
        {
            objects::GameObject* object;
            objects::GameObject* new_parent;
        };

        static inline objects::GameObject* selected_ = nullptr;
        static inline std::unordered_set<objects::GameObject*> selection_;
        static inline std::vector<PendingReparent> pending_reparents_;
        static inline std::unordered_set<objects::GameObject*> pending_open_;

        static void go_update(objects::GameObject* go);
        static int create_flags(objects::GameObject* go);
        static void update_childs(objects::GameObject* go);

        static void apply_selection_requests(void* ms_io);
        static void select_all_recursive(objects::GameObject* go);
        static void apply_range_recursive(objects::GameObject* go,
                                          objects::GameObject* first,
                                          objects::GameObject* last,
                                          bool selected, bool& in_range);

        static void reconcile_selected();
        static objects::GameObject* find_topmost_selected();
        static objects::GameObject*
        find_topmost_selected_recursive(objects::GameObject* go);

        static void handle_drag_drop(objects::GameObject* go);
        static void handle_root_drop_left(float top_left_x, float top_left_y,
                                          float height);
        static void handle_root_drop_bottom(float width, float height);
        static void
        queue_reparent_from_payload(objects::GameObject* new_parent);
        static void reparent_to(objects::GameObject* obj,
                                objects::GameObject* new_parent);
    };
} // namespace raphEngine::editor
