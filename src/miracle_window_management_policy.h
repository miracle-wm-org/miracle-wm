//
// Created by mattkae on 9/8/23.
//

#ifndef MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H
#define MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H

#include "window_tree.h"
#include <miral/window_manager_tools.h>
#include <miral/minimal_window_manager.h>
#include <miral/external_client.h>
#include <miral/internal_client.h>
#include <memory>

namespace miracle
{
class DisplayListener;
class TaskBar;

class MiracleWindowManagementPolicy : public miral::MinimalWindowManager
{
public:
    MiracleWindowManagementPolicy(
        miral::WindowManagerTools const&,
        miral::ExternalClientLauncher const&,
        miral::InternalClientLauncher const&);
    ~MiracleWindowManagementPolicy() = default;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;
    auto place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) -> miral::WindowSpecification override;
    void handle_window_ready(
        miral::WindowInfo& window_info) override;
    void advise_focus_gained(miral::WindowInfo const& window_info) override;
    void advise_focus_lost(miral::WindowInfo const& window_info) override;

private:
    WindowTree tree; // TODO: Keep a list per output
    miral::WindowManagerTools const window_manager_tools;
    miral::ExternalClientLauncher const external_client_launcher;
    miral::InternalClientLauncher const internal_client_launcher;
};
}

#endif //MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H
