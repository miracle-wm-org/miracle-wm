//
// Created by mattkae on 9/8/23.
//

#ifndef MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H
#define MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H

#include <miral/window_manager_tools.h>
#include <miral/minimal_window_manager.h>
#include <miral/external_client.h>
#include <memory>

namespace mirie
{
class TilingRegion;
class DisplayListener;

class MirieWindowManagementPolicy : public miral::MinimalWindowManager
{
public:
    MirieWindowManagementPolicy(
        miral::WindowManagerTools const&,
        miral::ExternalClientLauncher const&,
        mirie::DisplayListener& display_listener);
    ~MirieWindowManagementPolicy() = default;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

private:
    miral::WindowManagerTools const window_manager_tools;
    miral::ExternalClientLauncher const launcher;
    std::shared_ptr<TilingRegion> root;
    mirie::DisplayListener const display_listener;
};
}

#endif //MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H
