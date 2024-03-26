#ifndef MIRACLEWM_WINDOW_HELPERS_H
#define MIRACLEWM_WINDOW_HELPERS_H

#include <miral/window_info.h>
#include <miral/window_manager_tools.h>

namespace miracle
{
class LeafNode;
class Tree;
class WindowMetadata;

namespace window_helpers
{
bool is_window_fullscreen(MirWindowState state);

template <typename T>
bool is_tileable(T const& requested_specification)
{
    auto t = requested_specification.type();
    auto state = requested_specification.state();
    auto has_exclusive_rect = requested_specification.exclusive_rect().is_set();
    return (t == mir_window_type_normal || t == mir_window_type_freestyle)
           && (state == mir_window_state_restored || state == mir_window_state_maximized)
           && !has_exclusive_rect;
}

std::shared_ptr<WindowMetadata> get_metadata(
    miral::WindowInfo const& info);

std::shared_ptr<WindowMetadata> get_metadata(
    miral::Window const& window,
    miral::WindowManagerTools const& tools);

std::shared_ptr<LeafNode> get_node_for_window(
    miral::Window const& window,
    miral::WindowManagerTools const& tools);

std::shared_ptr<LeafNode> get_node_for_window_by_tree(
    miral::Window const& window,
    miral::WindowManagerTools const& tools,
    Tree const* tree);

miral::WindowSpecification copy_from(miral::WindowInfo const&);
}
}

#endif //MIRACLEWM_WINDOW_HELPERS_H
