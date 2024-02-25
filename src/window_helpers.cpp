#define MIR_LOG_COMPONENT "window_helpers"

#include "window_helpers.h"
#include "window_metadata.h"
#include "node.h"
#include <mir/log.h>

bool miracle::window_helpers::is_window_fullscreen(MirWindowState state)
{
    return state == mir_window_state_fullscreen
           || state == mir_window_state_maximized
           || state == mir_window_state_horizmaximized
           || state == mir_window_state_vertmaximized;
}

std::shared_ptr<miracle::Node> miracle::window_helpers::get_node_for_window(
    miral::Window const& window,
    miral::WindowManagerTools const& tools)
{
    auto& info = tools.info_for(window);
    if (info.userdata())
    {
        std::shared_ptr<WindowMetadata> data = static_pointer_cast<WindowMetadata>(info.userdata());
        return data->get_tiling_node();
    }

    mir::log_error("Unable to find node for window");
    return nullptr;
}

std::shared_ptr<miracle::Node> miracle::window_helpers::get_node_for_window_by_tree(
    const miral::Window &window,
    const miral::WindowManagerTools &tools,
    const miracle::Tree *tree)
{
    auto node = get_node_for_window(window, tools);
    if (node && node->get_tree() == tree)
    {
        return node;
    }

    return nullptr;
}