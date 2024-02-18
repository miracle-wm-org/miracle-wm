#ifndef MIRIACLE_WINDOW_MANAGEMENT_POLICY_H
#define MIRIACLE_WINDOW_MANAGEMENT_POLICY_H

#include "screen.h"
#include "miracle_config.h"
#include "workspace_manager.h"

#include <miral/window_manager_tools.h>
#include <miral/window_management_policy.h>
#include <miral/external_client.h>
#include <miral/internal_client.h>
#include <miral/output.h>
#include <memory>
#include <vector>

namespace miral
{
class MirRunner;
}

namespace miracle
{

struct OutputInfo
{
    miral::Output output;
    std::shared_ptr<Screen> screen;
};

class MiracleWindowManagementPolicy : public miral::WindowManagementPolicy
{
public:
    MiracleWindowManagementPolicy(
        miral::WindowManagerTools const&,
        miral::ExternalClientLauncher const&,
        miral::InternalClientLauncher const&,
        miral::MirRunner&,
        MiracleConfig const&);
    ~MiracleWindowManagementPolicy() override = default;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;
    bool handle_pointer_event(MirPointerEvent const* event) override;
    auto place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) -> miral::WindowSpecification override;
    void advise_new_window(miral::WindowInfo const& window_info) override;
    void handle_window_ready(miral::WindowInfo& window_info) override;
    void advise_focus_gained(miral::WindowInfo const& window_info) override;
    void advise_focus_lost(miral::WindowInfo const& window_info) override;
    void advise_delete_window(miral::WindowInfo const& window_info) override;
    void advise_move_to(miral::WindowInfo const& window_info, geom::Point top_left) override;
    void advise_output_create(miral::Output const& output) override;
    void advise_output_update(miral::Output const& updated, miral::Output const& original) override;
    void advise_output_delete(miral::Output const& output) override;
    void advise_state_change(miral::WindowInfo const& window_info, MirWindowState state) override;

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
    std::shared_ptr<OutputInfo> active_output;
    std::vector<std::shared_ptr<OutputInfo>> output_list;
    std::weak_ptr<OutputInfo> pending_output;
    std::vector<Window> orphaned_window_list;
    miral::WindowManagerTools window_manager_tools;
    miral::ExternalClientLauncher const external_client_launcher;
    miral::InternalClientLauncher const internal_client_launcher;
    miral::MirRunner& runner;
    MiracleConfig const& config;
    WorkspaceManager workspace_manager;

    void _add_to_output_immediately(Window&, std::shared_ptr<OutputInfo>&);
};
}

#endif //MIRIACLE_WINDOW_MANAGEMENT_POLICY_H
