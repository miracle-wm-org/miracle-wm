#define MIR_LOG_COMPONENT "window_helpers"

#include "window_helpers.h"
#include "window_metadata.h"
#include "node.h"
#include "leaf_node.h"
#include <mir/log.h>

bool miracle::window_helpers::is_window_fullscreen(MirWindowState state)
{
    return state == mir_window_state_fullscreen
           || state == mir_window_state_maximized
           || state == mir_window_state_horizmaximized
           || state == mir_window_state_vertmaximized;
}

std::shared_ptr<miracle::WindowMetadata> miracle::window_helpers::get_metadata(const miral::WindowInfo &info)
{
    if (info.userdata())
        return static_pointer_cast<WindowMetadata>(info.userdata());

    return nullptr;
}

std::shared_ptr<miracle::WindowMetadata>
miracle::window_helpers::get_metadata(const miral::Window &window, const miral::WindowManagerTools &tools)
{
    auto& info = tools.info_for(window);
    if (info.userdata())
        return static_pointer_cast<WindowMetadata>(info.userdata());

    return nullptr;
}

std::shared_ptr<miracle::LeafNode> miracle::window_helpers::get_node_for_window(
    miral::Window const& window,
    miral::WindowManagerTools const& tools)
{
    auto metadata = get_metadata(window, tools);
    if (metadata)
        return metadata->get_tiling_node();

    return nullptr;
}

std::shared_ptr<miracle::Node> miracle::window_helpers::get_node_for_window_by_tree(
    const miral::Window &window,
    const miral::WindowManagerTools &tools,
    const miracle::Tree *tree)
{
    auto node = get_node_for_window(window, tools);
    if (node && node->get_tree() == tree)
        return node;

    return nullptr;
}

miral::WindowSpecification miracle::window_helpers::copy_from(miral::WindowInfo const& info)
{
    miral::WindowSpecification spec;
    spec.name() = info.name();
    spec.state() = info.state();
    spec.type() = info.type();
    spec.parent() = info.parent();
    spec.min_width() = info.min_width();
    spec.max_width() = info.max_width();
    spec.min_height() = info.min_height();
    spec.max_height() = info.max_height();
    spec.width_inc() = info.width_inc();
    spec.height_inc() = info.height_inc();
    spec.min_aspect() = info.min_aspect();
    spec.max_aspect() = info.max_aspect();
    spec.preferred_orientation() = info.preferred_orientation();
    spec.confine_pointer() = info.confine_pointer();
    spec.shell_chrome() = info.shell_chrome();
    spec.userdata() = info.userdata();
    spec.attached_edges() = info.attached_edges();
    if (info.exclusive_rect().is_set())
    {
        spec.exclusive_rect() = mir::optional_value<mir::optional_value<geom::Rectangle>>(info.exclusive_rect());
    }
    spec.application_id() = info.application_id();
    spec.focus_mode() = info.focus_mode();
    spec.visible_on_lock_screen() = info.visible_on_lock_screen();
    return spec;
}