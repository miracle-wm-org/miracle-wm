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
#include <miral/output.h>
#include <memory>
#include <vector>

namespace miracle
{
class DisplayListener;
class TaskBar;

struct OutputTreePair
{
    miral::Output output;
    WindowTree tree;
};

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
    void advise_delete_window(miral::WindowInfo const& window_info) override;
    void advise_output_create(miral::Output const& output);
    void advise_output_update(miral::Output const& updated, miral::Output const& original);
    void advise_output_delete(miral::Output const& output);

private:
    std::vector<OutputTreePair> tree_list;
    miral::WindowManagerTools const window_manager_tools;
    miral::ExternalClientLauncher const external_client_launcher;
    miral::InternalClientLauncher const internal_client_launcher;
};
}

#endif //MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H
