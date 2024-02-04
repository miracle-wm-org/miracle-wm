#include "workspace_manager.h"
#include "screen.h"

using namespace mir::geometry;
using namespace miral;
using namespace miracle;

namespace
{
char DEFAULT_WORKSPACES[] = {'1','2','3','4','5','6','7','8','9', '0'};
}

WorkspaceManager::WorkspaceManager(WindowManagerTools const& tools) :
    tools_{tools}
{
}

bool WorkspaceManager::request_workspace(miracle::Screen *screen, char key)
{
    for (auto workspace : workspaces)
    {
        if (workspace.key == key)
        {
            // TODO: Select workspace
            return true;
        }
    }

    workspaces.push_back({
        key,
        screen
    });
    screen->advise_new_workspace(key);
    return true;
}

bool WorkspaceManager::request_first_available_workspace(miracle::Screen *screen)
{
    for (int i = 0; i < 10; i++)
    {
        bool can_use = true;
        for (auto workspace : workspaces)
        {
            if (workspace.key == DEFAULT_WORKSPACES[i])
            {
                can_use = false;
            }
        }

        if (can_use)
            return request_workspace(screen, DEFAULT_WORKSPACES[i]);
    }

    return false;
}


//
//void miracle::WorkspaceManager::jump_to_workspace(bool take_active, int index)
//{
//    tools_.invoke_under_lock(
//        [this, take_active, index]
//            {
//            if (active_workspace_index != index)
//            {
//                auto const old_workspace = active_workspace_index;
//                auto const& window = take_active ? tools_.active_window() : Window{};
//                auto const& old_active = workspaces[active_workspace_index];
//
//                while (workspaces.size() - 1 < index)
//                    workspaces.push_back(tools_.create_workspace());
//
//                auto const& new_active = workspaces[index];
//                change_active_workspace(new_active, old_active, window);
//                erase_if_empty(std::next(workspaces.begin(), old_workspace));
//                active_workspace_index = index;
//            }
//        });
//}
//
//void miracle::WorkspaceManager::workspace_begin(bool take_active)
//{
//    jump_to_workspace(take_active, 0);
//}
//
//void miracle::WorkspaceManager::workspace_end(bool take_active)
//{
//    jump_to_workspace(take_active, workspaces.size() - 1);
//}
//
//void miracle::WorkspaceManager::workspace_up(bool take_active)
//{
//    if (active_workspace_index != 0)
//        jump_to_workspace(take_active, active_workspace_index - 1);
//}
//
//void miracle::WorkspaceManager::workspace_down(bool take_active)
//{
//    if (active_workspace_index != workspaces.size() - 1)
//        jump_to_workspace(active_workspace_index + 1, take_active);
//}
//
//void miracle::WorkspaceManager::erase_if_empty(workspace_list::iterator const& old_workspace)
//{
//    bool empty = true;
//    tools_.for_each_window_in_workspace(*old_workspace, [&](auto ww)
//        {
//            if (is_application(tools_.info_for(ww).depth_layer()))
//                empty = false;
//        });
//    if (empty)
//    {
//        workspace_to_active.erase(*old_workspace);
//        workspaces.erase(old_workspace);
//    }
//}
//
//void miracle::WorkspaceManager::apply_workspace_hidden_to(Window const& window)
//{
//    auto const& window_info = tools_.info_for(window);
//    auto& workspace_info = workspace_info_for(window_info);
//    if (!workspace_info.in_hidden_workspace)
//    {
//        workspace_info.in_hidden_workspace = true;
//        workspace_info.old_state = window_info.state();
//
//        WindowSpecification modifications;
//        modifications.state() = mir_window_state_hidden;
//        tools_.place_and_size_for_state(modifications, window_info);
//        tools_.modify_window(window_info.window(), modifications);
//    }
//}
//
//void miracle::WorkspaceManager::apply_workspace_visible_to(Window const& window)
//{
//    auto const& window_info = tools_.info_for(window);
//    auto& workspace_info = workspace_info_for(window_info);
//    if (workspace_info.in_hidden_workspace)
//    {
//        workspace_info.in_hidden_workspace = false;
//        WindowSpecification modifications;
//        modifications.state() = workspace_info.old_state;
//        tools_.place_and_size_for_state(modifications, window_info);
//        tools_.modify_window(window_info.window(), modifications);
//    }
//}
//
//void miracle::WorkspaceManager::change_active_workspace(
//    std::shared_ptr<Workspace> const& new_active,
//    std::shared_ptr<Workspace> const& old_active,
//    Window const& window)
//{
//    if (new_active == old_active) return;
//
//    auto const old_active_window = tools_.active_window();
//    auto const old_active_window_shell = old_active_window &&
//        !is_application(tools_.info_for(old_active_window).depth_layer());
//
//    if (!old_active_window || old_active_window_shell)
//    {
//        // If there's no active window, the first shown grabs focus: get the right one
//        if (auto const ww = workspace_to_active[new_active])
//        {
//            tools_.for_each_workspace_containing(ww, [&](std::shared_ptr<Workspace> const& ws)
//            {
//                if (ws == new_active)
//                {
//                    apply_workspace_visible_to(ww);
//                }
//            });
//
//            // If focus was on a shell window, put it on an app
//            if (old_active_window_shell)
//                tools_.select_active_window(ww);
//        }
//    }
//
//    tools_.remove_tree_from_workspace(window, old_active);
//    tools_.add_tree_to_workspace(window, new_active);
//
//    tools_.for_each_window_in_workspace(new_active, [&](Window const& ww)
//    {
//        if (is_application(tools_.info_for(ww).depth_layer()))
//        {
//            apply_workspace_visible_to(ww);
//        }
//    });
//
//    bool hide_old_active = false;
//    tools_.for_each_window_in_workspace(old_active, [&](Window const& ww)
//    {
//        if (is_application(tools_.info_for(ww).depth_layer()))
//        {
//            if (ww == old_active_window)
//            {
//                // If we hide the active window focus will shift: do that last
//                hide_old_active = true;
//                return;
//            }
//
//            apply_workspace_hidden_to(ww);
//            return;
//        }
//    });
//
//    if (hide_old_active)
//    {
//        apply_workspace_hidden_to(old_active_window);
//
//        // Remember the old active_window when we switch away
//        workspace_to_active[old_active] = old_active_window;
//    }
//}
//
//void miracle::WorkspaceManager::advise_adding_to_workspace(std::shared_ptr<Workspace> const& workspace,
//                                                           std::vector<Window> const& windows)
//{
//    if (windows.empty())
//        return;
//
//    for (auto const& window : windows)
//    {
//        if (workspace == workspaces[active_workspace_index])
//        {
//            apply_workspace_visible_to(window);
//        }
//        else
//        {
//            apply_workspace_hidden_to(window);
//        }
//    }
//}
//
//auto miracle::WorkspaceManager::active_workspace() const -> std::shared_ptr<Workspace>
//{
//    return workspaces[active_workspace_index];
//}
//
//bool miracle::WorkspaceManager::is_application(MirDepthLayer layer)
//{
//    switch (layer)
//    {
//    case mir_depth_layer_application:
//    case mir_depth_layer_always_on_top:
//        return true;
//
//    default:;
//        return false;
//    }
//}
//
//bool miracle::WorkspaceManager::in_hidden_workspace(WindowInfo const& info) const
//{
//    auto& workspace_info = workspace_info_for(info);
//
//    return workspace_info.in_hidden_workspace;
//}
//
//void miracle::WorkspaceManager::advise_new_window(WindowInfo const& window_info)
//{
//    if (auto const& parent = window_info.parent())
//    {
//        if (workspace_info_for(tools_.info_for(parent)).in_hidden_workspace)
//            apply_workspace_hidden_to(window_info.window());
//    }
//    else
//    {
//        tools_.add_tree_to_workspace(window_info.window(), active_workspace());
//    }
//}
//
//auto miracle::WorkspaceManager::make_workspace_info() -> std::shared_ptr<WorkspaceInfo>
//{
//    return std::make_shared<WorkspaceInfo>();
//}
