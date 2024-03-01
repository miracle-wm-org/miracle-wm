#define MIR_LOG_COMPONENT "workspace_manager"
#include "workspace_manager.h"
#include "output_content.h"
#include "window_helpers.h"
#include <mir/log.h>

using namespace mir::geometry;
using namespace miral;
using namespace miracle;

WorkspaceManager::WorkspaceManager(
    WindowManagerTools const& tools,
    WorkspaceObserverRegistrar& registry,
    std::function<std::shared_ptr<OutputContent> const()> const& get_active_screen) :
    tools_{tools},
    registry{registry},
    get_active_screen{get_active_screen}
{
}

std::shared_ptr<OutputContent> WorkspaceManager::request_workspace(std::shared_ptr<OutputContent> screen, int key)
{
    if (workspaces[key] != nullptr)
    {
        auto workspace = workspaces[key];
        auto active_workspace = workspace->get_active_workspace_num();
        if (active_workspace == key)
        {
            mir::log_warning("Same workspace selected twice in a row");
            return workspace;
        }

        request_focus(key);
        return workspace;
    }

    workspaces[key] = screen;
    screen->advise_new_workspace(key);

    request_focus(key);
    registry.advise_created(workspaces[key], key);
    return screen;
}

bool WorkspaceManager::request_first_available_workspace(std::shared_ptr<OutputContent> screen)
{
    for (int i = 1; i < NUM_WORKSPACES; i++)
    {
        if (workspaces[i] == nullptr)
        {
            request_workspace(screen, i);
            return true;
        }
    }

    if (workspaces[0] == nullptr)
    {
        request_workspace(screen, 0);
        return true;
    }

    return false;
}

bool WorkspaceManager::move_active_to_workspace(std::shared_ptr<OutputContent> screen, int workspace)
{
    auto window = tools_.active_window();
    if (!window)
        return false;

    auto original_tree = screen->get_active_tree();
    auto metadata = window_helpers::get_metadata(window, tools_);
    auto window_node = window_helpers::get_node_for_window(window, tools_);
    original_tree->advise_delete_window(window);

    auto screen_to_move_to = request_workspace(screen, workspace);
    auto& prev_info = tools_.info_for(window);

    // TODO: These need to be set so that the window is correctly seen as tileable
    miral::WindowSpecification spec;
    spec.type() = prev_info.type();
    spec.state() = prev_info.state();
    spec = screen_to_move_to->get_active_tree()->allocate_position(spec);
    tools_.modify_window(window, spec);

    auto new_node = screen_to_move_to->get_active_tree()->advise_new_window(prev_info);
    metadata->associate_to_node(new_node);
    miral::WindowSpecification next_spec;
    next_spec.userdata() = metadata;
    tools_.modify_window(window, next_spec);

    screen_to_move_to->get_active_tree()->handle_window_ready(prev_info);
    return true;
}

bool WorkspaceManager::delete_workspace(int key)
{
    if (workspaces[key])
    {
        workspaces[key]->advise_workspace_deleted(key);
        registry.advise_removed(workspaces[key], key);
        workspaces[key] = nullptr;
        return true;
    }

    return false;
}

void WorkspaceManager::request_focus(int key)
{
    if (!workspaces[key])
        return;

    auto active_screen = get_active_screen();
    workspaces[key]->advise_workspace_active(key);

    if (active_screen != nullptr)
    {
        auto active_workspace = active_screen->get_active_workspace_num();
        registry.advise_focused(active_screen, active_workspace, workspaces[key], key);
    }
    else
        registry.advise_focused(nullptr, -1, workspaces[key], key);
}
