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

#include "spread_controller.h"

#include "compositor_state.h"
#include "config.h"
#include "constants.h"
#include "scene_override.h"
#include "spread_scene_override.h"

#include <miral/toolkit_event.h>

using namespace miracle;

SpreadController::SpreadController(
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<OutputManager> const& output_manager,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<Config> const& config) :
    compositor_state { compositor_state },
    output_manager { output_manager },
    animator { animator },
    window_controller { window_controller },
    config { config }
{
}

void SpreadController::break_tap()
{
    tap_latched = false;
}

void SpreadController::handle_keyboard_event(MirKeyboardEvent const* event, unsigned int modifiers)
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

void SpreadController::try_start()
{
    if (compositor_state->mode() != WindowManagerMode::normal)
        return;

    auto scene_override = SpreadSceneOverride::create(
        *output_manager,
        animator,
        window_controller,
        compositor_state,
        config,
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
        raw->start();
    }
}
