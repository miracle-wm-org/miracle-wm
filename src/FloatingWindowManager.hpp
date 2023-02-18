#ifndef FLOATING_WINDOW_MANAGER_HPP
#define FLOATING_WINDOW_MANAGER_HPP

#include "mir/geometry/rectangles.h"
#include <miral/window_management_policy.h>
#include <miral/window_manager_tools.h>
#include <mir_toolkit/events/enums.h>
#include <miral/minimal_window_manager.h>
#include <miral/application_info.h>
#include <miral/internal_client.h>
#include <miral/toolkit_event.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>
#include <miral/zone.h>

class FloatingWindowManager : public miral::WindowManagementPolicy {
public:
    FloatingWindowManager(const miral::WindowManagerTools&);
    ~FloatingWindowManager();
    miral::WindowSpecification place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) override;
    bool handle_pointer_event(MirPointerEvent const* event) override;
    bool handle_touch_event(MirTouchEvent const* event) override;
    bool handle_keyboard_event(MirKeyboardEvent const* event) override;
    void advise_new_window(miral::WindowInfo const& window_info) override;
    void handle_window_ready(miral::WindowInfo& window_info) override;
    void handle_raise_window (miral::WindowInfo &window_info) override;
    void advise_focus_gained(miral::WindowInfo const& info) override;
    void handle_modify_window(miral::WindowInfo& window_info, miral::WindowSpecification const& modifications) override;
    mir::geometry::Rectangle confirm_placement_on_display (miral::WindowInfo const &window_info, MirWindowState new_state, mir::geometry::Rectangle const &new_placement) override;
    void handle_request_drag_and_drop (miral::WindowInfo &window_info) override;
    void handle_request_move (miral::WindowInfo &window_info, MirInputEvent const *input_event) override;
    void handle_request_resize (miral::WindowInfo &window_info, MirInputEvent const *input_event, MirResizeEdge edge) override;
    mir::geometry::Rectangle confirm_inherited_move (miral::WindowInfo const &window_info, mir::geometry::Displacement movement) override;
};

#endif