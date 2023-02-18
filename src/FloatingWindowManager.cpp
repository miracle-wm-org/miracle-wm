#include "FloatingWindowManager.hpp"
#include <miral/minimal_window_manager.h>

const int title_bar_height = 16;

FloatingWindowManager::FloatingWindowManager(const miral::WindowManagerTools& tools) {

}

miral::WindowSpecification FloatingWindowManager::place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification)
{
    auto parameters = miral::MinimalWindowManager::place_new_window(app_info, requested_specification);

    // if (app_info.application() == miral::decoration_provider->session())
    // {
    //     parameters.type() = mir_window_type_decoration;
    //     parameters.depth_layer() = mir_depth_layer_background;
    // }

    bool const needs_titlebar = miral::WindowInfo::needs_titlebar(parameters.type().value());

    if (parameters.state().value() != mir_window_state_fullscreen && needs_titlebar)
        parameters.top_left() = miral::Point{parameters.top_left().value().x, parameters.top_left().value().y + title_bar_height};

    parameters.userdata() = std::make_shared<miral::PolicyData>();
    return parameters;
}