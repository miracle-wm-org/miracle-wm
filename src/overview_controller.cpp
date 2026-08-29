/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "overview_controller.h"

#include "abstract_command_controller.h"
#include "abstract_output.h"
#include "abstract_workspace.h"
#include "compositor_state.h"
#include "config.h"
#include "constants.h"
#include "output_manager.h"
#include "overview_scene_override.h"
#include "scene_override.h"

#include <miral/toolkit_event.h>

using namespace miracle;

OverviewController::OverviewController(
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<OutputManager> const& output_manager,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<AbstractCommandController> const& command_controller) :
    compositor_state { compositor_state },
    output_manager { output_manager },
    animator { animator },
    window_controller { window_controller },
    config { config },
    command_controller { command_controller }
{
}

void OverviewController::break_tap()
{
    tap_latched = false;
}

void OverviewController::handle_keyboard_event(MirKeyboardEvent const* event, unsigned int modifiers)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const keysym = miral::toolkit::mir_keyboard_event_keysym(event);
    auto const primary_modifier = config->get_input_event_modifier();

    bool const is_primary_key = is_modifier_keysym(primary_modifier, keysym);
    if (action == mir_keyboard_action_down)
        tap_latched = is_primary_key && modifiers == (static_cast<uint>(primary_modifier) & MODIFIER_MASK);
    else if (action == mir_keyboard_action_up && is_primary_key && tap_latched)
    {
        tap_latched = false;
        try_start();
    }
}

void OverviewController::try_start()
{
    if (compositor_state->mode() != WindowManagerMode::normal)
        return;

    auto scene_override = OverviewSceneOverride::create(
        *output_manager,
        animator,
        window_controller,
        compositor_state,
        config,
        [this]
    {
        // Runs on the main thread as soon as the overview begins going away, so
        // that the mode is broadcast from the main loop and the rest of the
        // compositor is usable again while the outro plays.
        command_controller->set_mode(WindowManagerMode::normal);
    },
        [this](uint32_t workspace_id)
    {
        // Runs when the overview's zoom has finished bringing this workspace up
        // to fill the output, so the switch itself must not animate.
        command_controller->select_workspace_by_id(workspace_id, false);
    },
        [this](int output_id)
    {
        // The overview's active group follows the cursor across outputs, but the
        // policy hands every pointer event straight to the override, so the
        // focused output has not moved with it. Dismissal has to bring the
        // output focus along, or [Policy::advise_focus_gained] drops the
        // selection as "not on the active workspace".
        auto const focused = output_manager->focused();
        if (focused && focused->id() == output_id)
            return;

        if (focused)
            output_manager->unfocus(focused->id());
        output_manager->focus(output_id);

        // The same thing the pointer path in [Policy::handle_pointer_event]
        // does after crossing to another output. The workspace is already
        // active there, so this only republishes the focus to the observers,
        // and the switch must not animate.
        for (auto const& output : output_manager->outputs())
        {
            if (output->id() != output_id)
                continue;

            if (auto const active = output->active())
                command_controller->select_workspace_by_id(active->id(), false);
            break;
        }
    },
        [this]
    {
        // Runs on the animator thread when the outro completes; only touches
        // the (thread-safe) manager and the atomic token.
        if (auto const released = token.exchange(0))
            compositor_state->scene_override_manager()->try_release_override(released);
    });
    if (!scene_override)
        return;

    auto* const raw = scene_override.get();
    if (auto const new_token = compositor_state->scene_override_manager()->try_override(std::move(scene_override)))
    {
        token.store(*new_token);
        command_controller->set_mode(WindowManagerMode::overview);
        raw->start();
    }
}
