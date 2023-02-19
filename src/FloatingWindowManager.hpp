#ifndef TILING_WINDOW_MANAGER_HPP
#define TILING_WINDOW_MANAGER_HPP

#include "WindowGroup.hpp"
#include <memory>
#include <miral/minimal_window_manager.h>
#include <mir_toolkit/events/enums.h>
#include <chrono>
#include <map>
#include <vector>

/** Defines how new windows will be placed in the workspace. */
enum class PlacementStrategy {
    /** If horizontal, we will place the new window to the right of the selectd window. */
    Horizontal,
    /** If vertical, we will place the new window below the selected window. */
    Vertical
};

namespace miral { class InternalClientLauncher; }

using namespace mir::geometry;

/**
* An implementation of a tiling window manager, much like i3.
*/
class FloatingWindowManagerPolicy : public miral::MinimalWindowManager {
public:
    FloatingWindowManagerPolicy(
        miral::WindowManagerTools const& tools,
        miral::InternalClientLauncher const& launcher,
        std::function<void()>& shutdown_hook);
    ~FloatingWindowManagerPolicy();

    /**
    * Positions the new window in reference to the currently selected window and the current mode.
    */
    virtual miral::WindowSpecification place_new_window(
        miral::ApplicationInfo const& app_info, miral::WindowSpecification const& request_parameters) override;


    bool handle_pointer_event(MirPointerEvent const* event) override;
    bool handle_touch_event(MirTouchEvent const* event) override;
    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    /** Add the window to the active zone. */
    void advise_new_window(miral::WindowInfo const& window_info) override;
    void handle_window_ready(miral::WindowInfo& window_info) override;
    void advise_focus_gained(miral::WindowInfo const& info) override;

    void handle_modify_window(miral::WindowInfo& window_info, miral::WindowSpecification const& modifications) override;

protected:
    static const int modifier_mask =
        mir_input_event_modifier_alt |
        mir_input_event_modifier_shift |
        mir_input_event_modifier_sym |
        mir_input_event_modifier_ctrl |
        mir_input_event_modifier_meta;

private:
    PlacementStrategy mActivePlacementStrategy;
    std::map<int, miral::Window> mZoneIdToWindowMap;
    WindowGroup mRootWindowGroup;
    std::shared_ptr<WindowGroup> mActiveWindowGroup;

    void changeStrategy(PlacementStrategy strategy);

    void toggle(MirWindowState state);

    int old_touch_pinch_top = 0;
    int old_touch_pinch_left = 0;
    int old_touch_pinch_width = 0;
    int old_touch_pinch_height = 0;
    bool pinching = false;

    void keep_window_within_constraints(
        miral::WindowInfo const& window_info,
        Displacement& movement,
        Width& new_width,
        Height& new_height) const;

    // Workaround for lp:1627697
    std::chrono::steady_clock::time_point last_resize;

    void advise_adding_to_workspace(
        std::shared_ptr<miral::Workspace> const& workspace,
        std::vector<miral::Window> const& windows) override;

    auto confirm_placement_on_display(
        miral::WindowInfo const& window_info,
        MirWindowState new_state,
        Rectangle const& new_placement) -> Rectangle override;

    // Switch workspace, taking window (if not null)
    void switch_workspace_to(
        std::shared_ptr<miral::Workspace> const& workspace,
        miral::Window const& window = miral::Window{});

    std::shared_ptr<miral::Workspace> active_workspace;
    std::map<int, std::shared_ptr<miral::Workspace>> key_to_workspace;
    std::map<std::shared_ptr<miral::Workspace>, miral::Window> workspace_to_active;

    void apply_workspace_visible_to(miral::Window const& window);

    void apply_workspace_hidden_to(miral::Window const& window);
};

#endif //TILING_WINDOW_MANAGER_HPP
