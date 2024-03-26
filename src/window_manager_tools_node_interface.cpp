#include "window_manager_tools_node_interface.h"
#include "window_helpers.h"

using namespace miracle;

WindowManagerToolsNodeInterface::WindowManagerToolsNodeInterface(
    miral::WindowManagerTools const& tools)
    : tools{tools}
{
}

bool WindowManagerToolsNodeInterface::is_fullscreen(miral::Window const& window)
{
    auto& info = tools.info_for(window);
    return window_helpers::is_window_fullscreen(info.state());
}

void WindowManagerToolsNodeInterface::set_rectangle(miral::Window const& window, geom::Rectangle const& r)
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

void WindowManagerToolsNodeInterface::show(miral::Window const& window)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_restored;
    tools.modify_window(window, spec);
}

void WindowManagerToolsNodeInterface::hide(miral::Window const& window)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_hidden;
    tools.modify_window(window, spec);
}

void WindowManagerToolsNodeInterface::clip(miral::Window const& window, geom::Rectangle const& r)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(r);
}

void WindowManagerToolsNodeInterface::noclip(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(mir::optional_value<geom::Rectangle>());
}