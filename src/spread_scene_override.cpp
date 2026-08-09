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

#include "spread_scene_override.h"

#include "animator.h"
#include "compositor_state.h"
#include "config.h"
#include "spread_layout.h"
#include "window_controller.h"

#include <algorithm>
#include <mir/scene/surface.h>
#include <miral/toolkit_event.h>
#include <xkbcommon/xkbcommon-keysyms.h>

using namespace miracle;
namespace geom = mir::geometry;

namespace
{
geom::Rectangle lerp_rect(geom::Rectangle const& from, geom::Rectangle const& to, float p)
{
    auto const lerp = [p](int a, int b)
    {
        return a + static_cast<int>(std::round(static_cast<float>(b - a) * p));
    };
    return geom::Rectangle {
        geom::Point {
                     lerp(from.top_left.x.as_value(), to.top_left.x.as_value()),
                     lerp(from.top_left.y.as_value(), to.top_left.y.as_value()) },
        geom::Size {
                     lerp(from.size.width.as_value(), to.size.width.as_value()),
                     lerp(from.size.height.as_value(), to.size.height.as_value()) }
    };
}

mir::scene::Surface const* surface_key(miral::Window const& window)
{
    return window.operator std::shared_ptr<mir::scene::Surface>().get();
}
}

bool miracle::is_modifier_keysym(MirInputEventModifier modifier, unsigned int keysym)
{
    switch (modifier)
    {
    case mir_input_event_modifier_meta:
        return keysym == XKB_KEY_Super_L || keysym == XKB_KEY_Super_R;
    case mir_input_event_modifier_alt:
        return keysym == XKB_KEY_Alt_L || keysym == XKB_KEY_Alt_R;
    case mir_input_event_modifier_ctrl:
        return keysym == XKB_KEY_Control_L || keysym == XKB_KEY_Control_R;
    case mir_input_event_modifier_shift:
        return keysym == XKB_KEY_Shift_L || keysym == XKB_KEY_Shift_R;
    default:
        return false;
    }
}

SpreadSceneOverride::SpreadSceneOverride(
    std::vector<Entry> entries,
    std::vector<geom::Rectangle> bounds,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<Config> const& config,
    std::function<void()>&& on_done) :
    state { std::make_shared<State>() },
    bounds { std::move(bounds) },
    animator { animator },
    window_controller { window_controller },
    compositor_state { compositor_state },
    on_done { std::move(on_done) },
    animation_handle { animator->register_animateable() },
    primary_modifier { config->get_input_event_modifier() }
{
    definition = config->get_animation_definition(AnimateableEvent::window_move);
    bool const usable = config->are_animations_enabled()
        && definition.duration_seconds > 0.f
        && !definition.data.empty()
        && definition.data[0].type != BultInAnimationType::disabled;
    if (!usable)
    {
        definition = AnimationDefinition {
            .is_default = true,
            .duration_seconds = 0.25f,
            .data = { BuiltInAnimationDefinition {
                .type = BultInAnimationType::slide,
                .function = EaseFunction::ease_out_cubic } }
        };
    }

    for (auto& entry : entries)
    {
        entry.from = entry.real;
        entry.current = entry.real;
        auto const* key = surface_key(entry.window);
        state->order.push_back(key);
        state->entries.emplace(key, std::move(entry));
    }
}

SpreadSceneOverride::~SpreadSceneOverride()
{
    animator->remove_by_animation_handle(animation_handle);
}

void SpreadSceneOverride::start()
{
    auto const s = state;
    animate([s]
    {
        std::lock_guard lock(s->mutex);
        if (s->phase == Phase::intro)
            s->phase = Phase::idle;
    });
}

void SpreadSceneOverride::animate(std::function<void()> on_complete)
{
    {
        std::lock_guard lock(state->mutex);
        state->t = 0.f;
    }
    animator->append(CustomAnimation {
        animation_handle,
        [weak = std::weak_ptr(state), definition = definition, on_complete = std::move(on_complete)](float dt) -> bool
        {
            auto const s = weak.lock();
            if (!s)
                return true;

            std::vector<std::shared_ptr<mir::scene::Surface>> to_nudge;
            bool done = false;
            {
                std::lock_guard lock(s->mutex);
                s->t = std::min(s->t + dt, definition.duration_seconds);
                float const p = ease(definition.data[0], s->t / definition.duration_seconds);
                for (auto& [key, entry] : s->entries)
                {
                    entry.current = lerp_rect(entry.from, entry.target, p);
                    if (auto const surface = entry.surface.lock())
                        to_nudge.push_back(surface);
                }
                done = s->t >= definition.duration_seconds;
            }

            // Re-applying the surfaces' own transformations marks the scene as
            // damaged so the compositor redraws with the new placements.
            for (auto const& surface : to_nudge)
                nudge(surface);

            if (done)
                on_complete();
            return done;
        } });
}

void SpreadSceneOverride::nudge(std::shared_ptr<mir::scene::Surface> const& surface)
{
    // Re-applying the current alpha notifies the surface observers
    // unconditionally, which marks the scene as damaged so the compositor
    // redraws with the new placements. (There is no way to query the current
    // transformation, so alpha is the value-preserving choice.)
    surface->set_alpha(surface->alpha());
}

void SpreadSceneOverride::begin_exit()
{
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        state->phase = Phase::outro;
        for (auto& [key, entry] : state->entries)
        {
            entry.from = entry.current;
            // The WM kept managing windows while the spread was open, so
            // re-read the live geometry as the outro target for a seamless
            // landing.
            entry.target = geom::Rectangle { entry.window.top_left(), entry.window.size() };
        }
    }

    auto const s = state;
    auto const done = on_done;
    animate([s, done]
    {
        {
            std::lock_guard lock(s->mutex);
            s->phase = Phase::done;
        }
        done();
    });
}

void SpreadSceneOverride::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const keysym = miral::toolkit::mir_keyboard_event_keysym(event);

    if (action == mir_keyboard_action_down && keysym == XKB_KEY_Escape)
    {
        begin_exit();
        return;
    }

    // Tapping the primary action modifier again toggles the spread closed.
    bool const is_primary = is_modifier_keysym(primary_modifier, keysym);
    if (action == mir_keyboard_action_down)
        primary_tap_latched = is_primary;
    else if (action == mir_keyboard_action_up && is_primary && primary_tap_latched)
    {
        primary_tap_latched = false;
        begin_exit();
    }
}

void SpreadSceneOverride::handle_pointer_event(MirPointerEvent const* event)
{
    float const x = miral::toolkit::mir_pointer_event_axis_value(event, mir_pointer_axis_x);
    float const y = miral::toolkit::mir_pointer_event_axis_value(event, mir_pointer_axis_y);
    auto const action = miral::toolkit::mir_pointer_event_action(event);

    if (action == mir_pointer_action_button_down)
        primary_tap_latched = false;

    std::optional<Entry> hit;
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        for (auto const* key : state->order)
        {
            auto const it = state->entries.find(key);
            if (it == state->entries.end())
                continue;
            if (it->second.current.contains(geom::Point { static_cast<int>(x), static_cast<int>(y) }))
            {
                hit = it->second;
                break;
            }
        }
    }

    if (action == mir_pointer_action_motion && hit)
    {
        auto const focused = compositor_state->focused_container();
        bool const already_focused = focused && focused->window() && focused->window().value() == hit->window;
        if (!already_focused)
        {
            window_controller->select_active_window(hit->window);

            // Focus changes only update the border color in the shared render
            // data; nudge the surfaces so the recolor shows immediately.
            if (focused && focused->window())
                if (auto const surface = focused->window().value().operator std::shared_ptr<mir::scene::Surface>())
                    nudge(surface);
            if (auto const surface = hit->surface.lock())
                nudge(surface);
        }
    }
    else if (action == mir_pointer_action_button_down)
    {
        if (hit)
            window_controller->select_active_window(hit->window);
        begin_exit();
    }
}

std::optional<SceneOverridePlacement> SpreadSceneOverride::place(miral::Window const& window)
{
    auto const* key = surface_key(window);
    std::lock_guard lock(state->mutex);
    auto const it = state->entries.find(key);
    if (it == state->entries.end())
        return std::nullopt;

    auto const& current = it->second.current;
    return SceneOverridePlacement {
        .position = {
                     static_cast<float>(current.top_left.x.as_value()),
                     static_cast<float>(current.top_left.y.as_value()) },
        .size = {
                     static_cast<float>(current.size.width.as_value()),
                     static_cast<float>(current.size.height.as_value()) },
        .transformation = glm::mat4(1.f)
    };
}

void SpreadSceneOverride::handle_window_added(miral::Window const& window)
{
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        auto const* key = surface_key(window);
        if (state->entries.contains(key))
            return;

        geom::Rectangle const real { window.top_left(), window.size() };

        // Assign the window to the output whose bounds contain its center;
        // fall back to the first output.
        geom::Point const center {
            real.top_left.x.as_value() + real.size.width.as_value() / 2,
            real.top_left.y.as_value() + real.size.height.as_value() / 2
        };
        size_t group = 0;
        for (size_t g = 0; g < bounds.size(); ++g)
        {
            if (bounds[g].contains(center))
            {
                group = g;
                break;
            }
        }

        Entry entry {
            .window = window,
            .surface = window,
            .real = real,
            .from = real,
            .target = real,
            .current = real,
            .group = group
        };
        state->order.insert(state->order.begin(), key);
        state->entries.emplace(key, std::move(entry));

        // Recompute the affected output's spread so the new window glides in
        // and the existing windows adjust around it. Other outputs keep their
        // targets; every entry still participates in the re-appended animation
        // so the whole scene stays coherent.
        std::vector<mir::scene::Surface const*> group_keys;
        std::vector<geom::Rectangle> reals;
        for (auto const* k : state->order)
        {
            auto const& e = state->entries.at(k);
            if (e.group == group)
            {
                group_keys.push_back(k);
                reals.push_back(e.real);
            }
        }

        auto const targets = spread_layout::compute(bounds[group], reals);
        for (auto& [k, e] : state->entries)
            e.from = e.current;
        for (size_t i = 0; i < group_keys.size(); ++i)
            state->entries.at(group_keys[i]).target = targets[i];
    }

    auto const s = state;
    animate([s]
    {
        std::lock_guard lock(s->mutex);
        if (s->phase == Phase::intro)
            s->phase = Phase::idle;
    });
}

void SpreadSceneOverride::handle_output_changed()
{
    cancel();
}

void SpreadSceneOverride::cancel()
{
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::done)
            return;

        state->phase = Phase::done;
    }

    animator->remove_by_animation_handle(animation_handle);
    on_done();
}

void SpreadSceneOverride::handle_window_closed(miral::Window const& window)
{
    std::lock_guard lock(state->mutex);
    auto const* key = surface_key(window);
    state->entries.erase(key);
    std::erase(state->order, key);
}
