//
// Created by mattkae on 9/8/23.
//

#ifndef MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H
#define MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H

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

private:
    miral::WindowManagerTools const window_manager_tools;
    miral::ExternalClientLauncher const external_client_launcher;
    miral::InternalClientLauncher const internal_client_launcher;
    std::shared_ptr<TaskBar> task_bar;
};
}

#endif //MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H
