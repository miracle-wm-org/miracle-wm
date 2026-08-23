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

#include "carousel_scene_override.h"

#include "animator.h"
#include "compositor_state.h"
#include "config.h"
#include "output.h"
#include "output_manager.h"
#include "window_container.h"
#include "window_controller.h"
#include "workspace.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <mir/scene/surface.h>
#include <miral/toolkit_event.h>
#include <miral/zone.h>
#include <unordered_map>
#include <xkbcommon/xkbcommon-keysyms.h>

using namespace miracle;
namespace geom = mir::geometry;

namespace
{
/// How long the carousel takes to settle after a scroll or a click. Deliberately
/// snappier than the configured window animation, so that a run of scroll
/// events does not lag behind the wheel.
constexpr float RETARGET_DURATION_SECONDS = 0.18f;

carousel_layout::Placement placement_of(geom::Rectangle const& rectangle)
{
    return carousel_layout::Placement {
        .x = static_cast<float>(rectangle.top_left.x.as_value()),
        .y = static_cast<float>(rectangle.top_left.y.as_value()),
        .width = static_cast<float>(rectangle.size.width.as_value()),
        .height = static_cast<float>(rectangle.size.height.as_value()),
        .opacity = 1.f
    };
}

/// The vertical scroll in \p event, measured in whole carousel steps.
///
/// A wheel reports high-resolution 120ths of a click, older ones report whole
/// clicks, and touchpads report a continuous tick count. Only one of the three
/// axes is populated for any given device, so they are tried in that order.
float vertical_scroll(MirPointerEvent const* event)
{
    auto const axis = [event](MirPointerAxis which)
    {
        return miral::toolkit::mir_pointer_event_axis_value(event, which);
    };

    if (auto const value120 = axis(mir_pointer_axis_vscroll_value120); value120 != 0.f)
        return value120 / 120.f;

    if (auto const discrete = axis(mir_pointer_axis_vscroll_discrete); discrete != 0.f)
        return discrete;

    return axis(mir_pointer_axis_vscroll);
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

bool miracle::is_carousel_window(
    std::shared_ptr<WindowContainer> const& container,
    WindowController& window_controller)
{
    if (!container || !container->window())
        return false;

    if (!container->get_workspace())
        return false;

    if (container->anchored())
        return true;

    auto const& info = window_controller.info_for(container->window().value());
    if (info.parent())
        return false;

    auto const type = info.type();
    return type == mir_window_type_normal || type == mir_window_type_freestyle;
}

std::unique_ptr<CarouselSceneOverride> CarouselSceneOverride::create(
    OutputManager& output_manager,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<Config> const& config,
    std::function<void()>&& on_exit_started,
    std::function<void()>&& on_done)
{
    // Every output runs a carousel of its own active workspace within its own
    // bounds.
    std::vector<geom::Rectangle> bounds_list;
    std::unordered_map<AbstractWorkspace const*, size_t> group_of_workspace;
    auto const focused_output = output_manager.focused();
    size_t active_group = 0;
    for (auto const& output : output_manager.outputs())
    {
        if (output->is_defunct())
            continue;

        auto const workspace = output->active();
        if (!workspace)
            continue;

        // Prefer the application zone so the carousel avoids panels and docks.
        auto bounds = output->get_area();
        if (!output->get_app_zones().empty())
            bounds = output->get_app_zones().front().extents();

        if (output == focused_output)
            active_group = bounds_list.size();

        group_of_workspace[workspace.get()] = bounds_list.size();
        bounds_list.push_back(bounds);
    }

    // The focus order is already most-recently-used first, which is the order
    // the carousel wants - the focused window lands at index 0 of its group and
    // therefore starts front and center - and unlike
    // [AbstractWorkspace::for_each_window] it does not filter out floating and
    // plugin-managed windows.
    std::vector<Entry> entries;
    std::vector<size_t> next_index(bounds_list.size(), 0);
    for (auto const& weak : compositor_state->windows())
    {
        auto const container = weak.lock();
        if (!is_carousel_window(container, *window_controller))
            continue;

        // A miss means the window lives on a workspace that is not part of the
        // carousel (i.e. a background workspace).
        auto const it = group_of_workspace.find(container->get_workspace().get());
        if (it == group_of_workspace.end())
            continue;

        auto const window = container->window().value();
        entries.push_back(Entry {
            .window = window,
            .real = geom::Rectangle { window.top_left(), window.size() },
            .group = it->second,
            .index = next_index[it->second]++
        });
    }

    if (entries.empty())
        return nullptr;

    return std::unique_ptr<CarouselSceneOverride>(new CarouselSceneOverride(
        std::move(entries),
        std::move(bounds_list),
        active_group,
        animator,
        window_controller,
        compositor_state,
        config,
        std::move(on_exit_started),
        std::move(on_done)));
}

CarouselSceneOverride::CarouselSceneOverride(
    std::vector<Entry> entries,
    std::vector<geom::Rectangle> bounds,
    size_t active_group,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<Config> const& config,
    std::function<void()>&& on_exit_started,
    std::function<void()>&& on_done) :
    state { std::make_shared<State>() },
    bounds { std::move(bounds) },
    animator { animator },
    window_controller { window_controller },
    compositor_state { compositor_state },
    on_exit_started { std::move(on_exit_started) },
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

    state->groups.resize(this->bounds.size());
    state->active_group = active_group;
    for (auto& entry : entries)
    {
        // The intro slides each window in from wherever it really is.
        entry.from = entry.current = placement_of(entry.real);
        state->groups[entry.group].count++;

        auto const* key = surface_key(entry.window);
        state->order.push_back(key);
        state->entries.emplace(key, std::move(entry));
    }

    for (size_t group = 0; group < state->groups.size(); ++group)
        retarget(group);
}

CarouselSceneOverride::~CarouselSceneOverride()
{
    animator->remove_by_animation_handle(animation_handle);
}

std::vector<mir::scene::Surface const*> CarouselSceneOverride::keys_of(size_t group) const
{
    std::vector<mir::scene::Surface const*> keys;
    for (auto const& [key, entry] : state->entries)
    {
        if (entry.group == group)
            keys.push_back(key);
    }

    std::ranges::sort(keys, [this](auto const* a, auto const* b)
    {
        return state->entries.at(a).index < state->entries.at(b).index;
    });
    return keys;
}

std::optional<miral::Window> CarouselSceneOverride::centered_window(size_t group) const
{
    if (group >= state->groups.size())
        return std::nullopt;

    auto const position = state->groups[group].position;
    for (auto const& [key, entry] : state->entries)
    {
        if (entry.group == group && entry.index == position)
            return entry.window;
    }

    return std::nullopt;
}

void CarouselSceneOverride::retarget(size_t group)
{
    auto const keys = keys_of(group);
    std::vector<geom::Rectangle> reals;
    reals.reserve(keys.size());
    for (auto const* key : keys)
        reals.push_back(state->entries.at(key).real);

    auto const targets = carousel_layout::compute(
        bounds[group], reals, static_cast<float>(state->groups[group].position));

    // Every entry - not just this group's - restarts from where it is right
    // now, because re-appending on the animation handle cancels the animation
    // that the other groups may still be part way through.
    for (auto& [key, entry] : state->entries)
        entry.from = entry.current;

    for (size_t i = 0; i < keys.size(); ++i)
        state->entries.at(keys[i]).target = targets[i];
}

bool CarouselSceneOverride::advance(size_t group, int delta)
{
    auto& carousel = state->groups[group];
    if (carousel.count == 0)
        return false;

    // The strip has ends: running off either one simply stops there.
    auto const last = static_cast<long>(carousel.count) - 1;
    auto const next = std::clamp(static_cast<long>(carousel.position) + delta, 0L, last);
    if (next == static_cast<long>(carousel.position))
        return false;

    carousel.position = static_cast<size_t>(next);
    retarget(group);
    return true;
}

void CarouselSceneOverride::step(int delta)
{
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        if (!advance(state->active_group, delta))
            return;
    }

    settle();
}

void CarouselSceneOverride::settle()
{
    auto const s = state;
    animate(
        [s]
    {
        std::lock_guard lock(s->mutex);
        if (s->phase == Phase::intro)
            s->phase = Phase::idle;
    },
        RETARGET_DURATION_SECONDS);
}

void CarouselSceneOverride::start()
{
    auto const s = state;
    animate([s]
    {
        std::lock_guard lock(s->mutex);
        if (s->phase == Phase::intro)
            s->phase = Phase::idle;
    });
}

void CarouselSceneOverride::animate(std::function<void()> on_complete, std::optional<float> duration)
{
    auto scoped = definition;
    if (duration)
        scoped.duration_seconds = std::min(*duration, definition.duration_seconds);

    {
        std::lock_guard lock(state->mutex);
        state->t = 0.f;
    }
    animator->append(CustomAnimation {
        animation_handle,
        [weak = std::weak_ptr(state), definition = scoped, on_complete = std::move(on_complete)](float dt) -> bool
    {
        auto const s = weak.lock();
        if (!s)
            return true;

        std::vector<miral::Window> to_nudge;
        bool done = false;
        {
            std::lock_guard lock(s->mutex);
            s->t = std::min(s->t + dt, definition.duration_seconds);
            float const p = ease(definition.data[0], s->t / definition.duration_seconds);
            for (auto& [key, entry] : s->entries)
            {
                entry.current = carousel_layout::lerp(entry.from, entry.target, p);
                to_nudge.push_back(entry.window);
            }
            done = s->t >= definition.duration_seconds;
        }

        // Re-applying the surfaces' own transformations marks the scene as
        // damaged so the compositor redraws with the new placements.
        for (auto const& window : to_nudge)
            nudge(window);

        if (done)
            on_complete();
        return done;
    } });
}

void CarouselSceneOverride::nudge(miral::Window const& window)
{
    // Re-applying the current alpha notifies the surface observers
    // unconditionally, which marks the scene as damaged so the compositor
    // redraws with the new placements. (There is no way to query the current
    // transformation, so alpha is the value-preserving choice.)
    if (auto const surface = window.operator std::shared_ptr<mir::scene::Surface>())
        surface->set_alpha(surface->alpha());
}

void CarouselSceneOverride::commit_and_exit()
{
    std::optional<miral::Window> centered;
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        centered = centered_window(state->active_group);
    }

    if (centered)
        window_controller->select_active_window(*centered);

    begin_exit();
}

void CarouselSceneOverride::begin_exit()
{
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        state->phase = Phase::outro;
        for (auto& [key, entry] : state->entries)
        {
            entry.from = entry.current;
            // The WM kept managing windows while the carousel was open, so
            // re-read the live geometry as the outro target for a seamless
            // landing. Opacity goes back to 1 so the dimmed windows brighten as
            // they settle.
            entry.target = placement_of(geom::Rectangle { entry.window.top_left(), entry.window.size() });
        }
    }

    // The carousel no longer owns the desktop from here on, even though the
    // outro is still playing. The phase guard above makes this fire exactly
    // once.
    on_exit_started();

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

void CarouselSceneOverride::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const keysym = miral::toolkit::mir_keyboard_event_keysym(event);

    if (action == mir_keyboard_action_down && keysym == XKB_KEY_Escape)
    {
        commit_and_exit();
        return;
    }

    // The arrow keys walk the carousel along, exactly as the scroll wheel does.
    // Repeats count, so holding an arrow down runs along the strip.
    if ((action == mir_keyboard_action_down || action == mir_keyboard_action_repeat)
        && (keysym == XKB_KEY_Left || keysym == XKB_KEY_Right))
    {
        primary_tap_latched = false;
        step(keysym == XKB_KEY_Right ? 1 : -1);
        return;
    }

    // Tapping the primary action modifier again dismisses the carousel the same
    // way that clicking the centered window does.
    bool const is_primary = is_modifier_keysym(primary_modifier, keysym);
    if (action == mir_keyboard_action_down)
        primary_tap_latched = is_primary;
    else if (action == mir_keyboard_action_up && is_primary && primary_tap_latched)
    {
        primary_tap_latched = false;
        commit_and_exit();
    }
}

void CarouselSceneOverride::handle_pointer_event(MirPointerEvent const* event)
{
    float const x = miral::toolkit::mir_pointer_event_axis_value(event, mir_pointer_axis_x);
    float const y = miral::toolkit::mir_pointer_event_axis_value(event, mir_pointer_axis_y);
    float const vscroll = vertical_scroll(event);
    auto const action = miral::toolkit::mir_pointer_event_action(event);

    if (action == mir_pointer_action_button_down)
        primary_tap_latched = false;

    bool needs_animation = false;
    bool commit = false;
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        // Scrolling and dismissal act on the carousel the cursor is over.
        for (size_t group = 0; group < bounds.size(); ++group)
        {
            if (bounds[group].contains(geom::Point { static_cast<int>(x), static_cast<int>(y) }))
            {
                state->active_group = group;
                break;
            }
        }

        auto& active = state->groups[state->active_group];
        if (vscroll != 0.f)
        {
            active.scroll_accumulator += vscroll;
            auto const steps = static_cast<int>(active.scroll_accumulator);
            if (steps != 0)
            {
                active.scroll_accumulator -= static_cast<float>(steps);

                // A positive vertical scroll is a scroll down, which advances
                // the carousel to the next window.
                needs_animation |= advance(state->active_group, steps);
            }
        }

        if (action == mir_pointer_action_button_down)
        {
            Entry const* hit = nullptr;
            for (auto const* key : state->order)
            {
                auto const it = state->entries.find(key);
                if (it == state->entries.end())
                    continue;
                if (carousel_layout::contains(it->second.current, x, y))
                {
                    hit = &it->second;
                    break;
                }
            }

            if (hit)
                state->active_group = hit->group;

            auto& group = state->groups[state->active_group];
            if (hit && hit->index != group.position)
            {
                // A window off to one side was picked: bring it front and
                // center rather than dismissing.
                group.scroll_accumulator = 0.f;
                needs_animation |= advance(
                    state->active_group, static_cast<int>(hit->index) - static_cast<int>(group.position));
            }
            else
            {
                // The centered window, or empty space: take what is centered.
                commit = true;
            }
        }
    }

    if (commit)
    {
        commit_and_exit();
        return;
    }

    if (needs_animation)
        settle();
}

std::optional<SceneOverridePlacement> CarouselSceneOverride::place(miral::Window const& window)
{
    auto const* key = surface_key(window);
    std::lock_guard lock(state->mutex);
    auto const it = state->entries.find(key);
    if (it == state->entries.end())
        return std::nullopt;

    auto const& current = it->second.current;
    auto const& real = it->second.real;

    // The window is never resized to fit its slot: it is relocated so that its
    // real rectangle is centered on the slot, and the shrink that makes it fit
    // is carried by the transformation.
    float const real_width = static_cast<float>(std::max(real.size.width.as_value(), 1));
    float const real_height = static_cast<float>(std::max(real.size.height.as_value(), 1));

    return SceneOverridePlacement {
        .position = { current.x + (current.width - real_width) / 2.f,
                     current.y + (current.height - real_height) / 2.f },
        .transformation = glm::scale(
            glm::mat4(1.f), glm::vec3(current.width / real_width, current.height / real_height, 1.f)),
        .opacity = current.opacity
    };
}

void CarouselSceneOverride::handle_window_added(miral::Window const& window)
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

        // The new window joins at the front of the carousel, which shifts every
        // other window on that output along by one. Shifting the position by
        // one too keeps whatever the user was looking at front and center.
        for (auto& [k, e] : state->entries)
        {
            if (e.group == group)
                e.index++;
        }

        auto const placement = placement_of(real);
        state->entries.emplace(key, Entry { .window = window, .real = real, .from = placement, .target = placement, .current = placement, .group = group, .index = 0 });
        state->order.insert(state->order.begin(), key);
        state->groups[group].count++;
        state->groups[group].position++;

        // Recompute the affected output's carousel so the new window glides in
        // and the existing windows shuffle around it. Other outputs keep their
        // targets; every entry still participates in the re-appended animation
        // so the whole scene stays coherent.
        retarget(group);
    }

    auto const s = state;
    animate([s]
    {
        std::lock_guard lock(s->mutex);
        if (s->phase == Phase::intro)
            s->phase = Phase::idle;
    });
}

void CarouselSceneOverride::handle_output_changed()
{
    cancel();
}

void CarouselSceneOverride::cancel()
{
    bool was_exiting = false;
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::done)
            return;

        was_exiting = state->phase == Phase::outro;
        state->phase = Phase::done;
    }

    // [begin_exit] has already announced the exit if the outro was running.
    if (!was_exiting)
        on_exit_started();

    animator->remove_by_animation_handle(animation_handle);
    on_done();
}

void CarouselSceneOverride::handle_window_closed(miral::Window const& window)
{
    size_t group = 0;
    bool empty = false;
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        auto const* key = surface_key(window);
        auto const it = state->entries.find(key);
        if (it == state->entries.end())
            return;

        group = it->second.group;
        auto const index = it->second.index;
        state->entries.erase(it);
        std::erase(state->order, key);

        // Close the gap the window left behind, and keep the position pointing
        // at a window that still exists.
        for (auto& [k, e] : state->entries)
        {
            if (e.group == group && e.index > index)
                e.index--;
        }

        auto& carousel = state->groups[group];
        carousel.count--;
        if (carousel.count == 0)
            carousel.position = 0;
        else if (carousel.position > index || carousel.position >= carousel.count)
            carousel.position--;

        empty = state->entries.empty();
        if (!empty)
            retarget(group);
    }

    if (empty)
    {
        // Nothing left to show.
        cancel();
        return;
    }

    auto const s = state;
    animate([s]
    {
        std::lock_guard lock(s->mutex);
        if (s->phase == Phase::intro)
            s->phase = Phase::idle;
    });
}
