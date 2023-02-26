#ifndef TILING_WINDOW_MANAGER_HPP
#define TILING_WINDOW_MANAGER_HPP

#include "TileNode.hpp"
#include "miral/window_management_policy.h"
#include "miral/window_specification.h"
#include <memory>
#include <miral/minimal_window_manager.h>
#include <mir_toolkit/events/enums.h>
#include <chrono>
#include <map>
#include <vector>

namespace miral {
    class InternalClientLauncher;
}

/**
* An implementation of a tiling window manager, much like i3.
*/
class TilingWindowManagerPolicy : public miral::MinimalWindowManager {
public:
    TilingWindowManagerPolicy(
        miral::WindowManagerTools const&,
        miral::InternalClientLauncher const&,
        std::function<void()>&);
    ~TilingWindowManagerPolicy();

    /**
    * Positions the new window in reference to the currently selected window and the current mode.
    */
    virtual miral::WindowSpecification place_new_window(
        miral::ApplicationInfo const&, miral::WindowSpecification const&) override;


    bool handle_pointer_event(MirPointerEvent const*) override;
    bool handle_touch_event(MirTouchEvent const*) override;
    bool handle_keyboard_event(MirKeyboardEvent const*) override;

    /** Add the window to the active zone. */
    void advise_new_window(miral::WindowInfo const&) override;
    void handle_window_ready(miral::WindowInfo&) override;
    void advise_focus_gained(miral::WindowInfo const&) override;

    void handle_modify_window(miral::WindowInfo&, miral::WindowSpecification const&) override;

protected:
    static const int pModifierMask =
        mir_input_event_modifier_alt |
        mir_input_event_modifier_shift |
        mir_input_event_modifier_sym |
        mir_input_event_modifier_ctrl |
        mir_input_event_modifier_meta;

private:
    std::shared_ptr<TileNode> mRootTileNode;
    std::shared_ptr<TileNode> mActiveTileNode;
    PlacementStrategy mDefaultStrategy;
    std::vector<std::shared_ptr<miral::Window>> mWindowsOnDesktop;
    std::shared_ptr<miral::Window> mActiveWindow;

    void requestPlacementStrategyChange(PlacementStrategy);
    void requestQuitSelectedApplication();
    bool requestChangeActiveWindow(int);
};

#endif //TILING_WINDOW_MANAGER_HPP
