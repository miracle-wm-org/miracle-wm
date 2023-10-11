//
// Created by mattkae on 9/8/23.
//
#define MIR_LOG_COMPONENT "mirie"

#include "mirie_window_management_policy.h"
#include <mir_toolkit/events/enums.h>
#include <miral/toolkit_event.h>
#include <mir/log.h>
#include <linux/input.h>


using namespace mirie;

namespace
{
const int MODIFIER_MASK =
    mir_input_event_modifier_alt |
    mir_input_event_modifier_shift |
    mir_input_event_modifier_sym |
    mir_input_event_modifier_ctrl |
    mir_input_event_modifier_meta;

const std::string TERMINAL = "xfce4-terminal";
}

MirieWindowManagementPolicy::MirieWindowManagementPolicy(
    const miral::WindowManagerTools & tools,
    miral::ExternalClientLauncher const& launcher)
    : miral::MinimalWindowManager(tools),
      window_manager_tools{tools},
      launcher{launcher}
{
}

bool MirieWindowManagementPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    if (MinimalWindowManager::handle_keyboard_event(event)) {
        return true;
    }

    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const scan_code = miral::toolkit::mir_keyboard_event_scan_code(event);
    auto const modifiers = miral::toolkit::mir_keyboard_event_modifiers(event) & MODIFIER_MASK;

    if (action == MirKeyboardAction::mir_keyboard_action_down && (modifiers && mir_input_event_modifier_alt)) {
        if (scan_code == KEY_ENTER) {
            launcher.launch({TERMINAL});
            return true;
        }
    }

    return false;
}