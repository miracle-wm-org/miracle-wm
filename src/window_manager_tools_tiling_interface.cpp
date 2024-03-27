#include "window_manager_tools_tiling_interface.h"
#include "window_helpers.h"
#include "window_metadata.h"
#include "leaf_node.h"

using namespace miracle;

WindowManagerToolsTilingInterface::WindowManagerToolsTilingInterface(
    miral::WindowManagerTools const& tools)
    : tools{tools}
{
}

bool WindowManagerToolsTilingInterface::is_fullscreen(miral::Window const& window)
{
    auto& info = tools.info_for(window);
    return window_helpers::is_window_fullscreen(info.state());
}

void WindowManagerToolsTilingInterface::set_rectangle(miral::Window const& window, geom::Rectangle const& r)
{
    miral::WindowSpecification spec;
    spec.top_left() = r.top_left;
    spec.size() = r.size;
    tools.modify_window(window, spec);

    auto& window_info = tools.info_for(window);
    for (auto const& child : window_info.children())
    {
        miral::WindowSpecification sub_spec;
        sub_spec.top_left() = r.top_left;
        sub_spec.size() = r.size;
        tools.modify_window(child, sub_spec);
    }
}

MirWindowState WindowManagerToolsTilingInterface::get_state(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    return window_info.state();
}

void WindowManagerToolsTilingInterface::change_state(miral::Window const& window, MirWindowState state)
{
    auto& window_info = tools.info_for(window);
    miral::WindowSpecification spec;
    spec.state() = state;
    tools.modify_window(window, spec);
    tools.place_and_size_for_state(spec, window_info);
}

void WindowManagerToolsTilingInterface::clip(miral::Window const& window, geom::Rectangle const& r)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(r);
}

void WindowManagerToolsTilingInterface::noclip(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(mir::optional_value<geom::Rectangle>());
}

void WindowManagerToolsTilingInterface::select_active_window(miral::Window const& window)
{
    tools.select_active_window(window);
}

std::shared_ptr<WindowMetadata> WindowManagerToolsTilingInterface::get_metadata(miral::Window const& window)
{
    auto& info = tools.info_for(window);
    if (info.userdata())
        return static_pointer_cast<WindowMetadata>(info.userdata());

    return nullptr;
}

std::shared_ptr<WindowMetadata> WindowManagerToolsTilingInterface::get_metadata(
    miral::Window const& window, TilingWindowTree const* tree)
{
    auto node = get_metadata(window);
    if (auto tiling_node = node->get_tiling_node())
    {
        if (tiling_node->get_tree() == tree)
            return node;
    }

    return nullptr;
}

void WindowManagerToolsTilingInterface::raise(miral::Window const& window)
{
    tools.raise_tree(window);
}

void WindowManagerToolsTilingInterface::send_to_back(miral::Window const& window)
{
    tools.send_tree_to_back(window);
}