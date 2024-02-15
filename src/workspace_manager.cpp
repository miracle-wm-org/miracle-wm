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

std::shared_ptr<Screen> WorkspaceManager::request_workspace(std::shared_ptr<Screen> screen, char key)
{
    for (auto workspace : workspaces)
    {
        if (workspace.key == key)
        {
            workspace.screen->make_workspace_active(key);
            return workspace.screen;
        }
    }

    workspaces.push_back({
        key,
        screen
    });
    screen->advise_new_workspace(key);
    return screen;
}

bool WorkspaceManager::request_first_available_workspace(std::shared_ptr<Screen> screen)
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
        {
            request_workspace(screen, DEFAULT_WORKSPACES[i]);
            return true;
        }
    }

    return false;
}

bool WorkspaceManager::move_active_to_workspace(std::shared_ptr<Screen> screen, char workspace)
{
    auto window = tools_.active_window();
    if (!window)
        return false;

    auto& original_tree = screen->get_active_tree();
    auto window_node = original_tree.get_root_node()->find_node_for_window(window);
    original_tree.advise_delete_window(window);

    auto screen_to_move_to = request_workspace(screen, workspace);
    auto& prev_info = tools_.info_for(window);

    // TODO: These need to be set so that the window is correctly seen as tileable
    miral::WindowSpecification spec;
    spec.type() = prev_info.type();
    spec.state() = prev_info.state();
    spec = screen_to_move_to->get_active_tree().allocate_position(spec);
    tools_.modify_window(window, spec);
    screen_to_move_to->get_active_tree().advise_new_window(prev_info);
    screen_to_move_to->get_active_tree().handle_window_ready(prev_info);
    return true;
}