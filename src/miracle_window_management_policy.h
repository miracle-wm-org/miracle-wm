#ifndef MIRIE_WINDOW_MANAGEMENT_POLICY_H
#define MIRIE_WINDOW_MANAGEMENT_POLICY_H

#include "window_tree.h"
#include <miral/window_manager_tools.h>
#include <miral/window_management_policy.h>
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

class MiracleWindowManagementPolicy : public miral::WindowManagementPolicy
{
public:
    MiracleWindowManagementPolicy(
        miral::WindowManagerTools const&,
        miral::ExternalClientLauncher const&,
        miral::InternalClientLauncher const&);
    ~MiracleWindowManagementPolicy() = default;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;
    bool handle_pointer_event(MirPointerEvent const* event) override;
    auto place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) -> miral::WindowSpecification override;
    void advise_new_window(miral::WindowInfo const& window_info) override;
    void handle_window_ready(
        miral::WindowInfo& window_info) override;
    void advise_focus_gained(miral::WindowInfo const& window_info) override;
    void advise_focus_lost(miral::WindowInfo const& window_info) override;
    void advise_delete_window(miral::WindowInfo const& window_info) override;
    void advise_resize(miral::WindowInfo const& window_info, geom::Size const& new_size) override;
    void advise_output_create(miral::Output const& output);
    void advise_output_update(miral::Output const& updated, miral::Output const& original);
    void advise_output_delete(miral::Output const& output);

    void handle_modify_window(miral::WindowInfo &window_info, const miral::WindowSpecification &modifications) override;

    void handle_raise_window(miral::WindowInfo &window_info) override;

    auto confirm_placement_on_display(
        const miral::WindowInfo &window_info,
        MirWindowState new_state,
        const mir::geometry::Rectangle &new_placement) -> mir::geometry::Rectangle override;

    bool handle_touch_event(const MirTouchEvent *event) override;

    void handle_request_move(miral::WindowInfo &window_info, const MirInputEvent *input_event) override;

    void handle_request_resize(
        miral::WindowInfo &window_info,
        const MirInputEvent *input_event,
        MirResizeEdge edge) override;

    auto confirm_inherited_move(
        const miral::WindowInfo &window_info,
        mir::geometry::Displacement movement) -> mir::geometry::Rectangle override;

    void advise_application_zone_create(miral::Zone const& application_zone) override;
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original) override;
    void advise_application_zone_delete(miral::Zone const& application_zone) override;

private:
    std::shared_ptr<OutputTreePair> active_tree;
    std::vector<std::shared_ptr<OutputTreePair>> tree_list;
    miral::WindowManagerTools window_manager_tools;
    miral::ExternalClientLauncher const external_client_launcher;
    miral::InternalClientLauncher const internal_client_launcher;

    void constrain_window(miral::WindowSpecification&, miral::WindowInfo&);
};
}

#endif //MIRCOMPOSITOR_MIRIE_WINDOW_MANAGEMENT_POLICY_H
