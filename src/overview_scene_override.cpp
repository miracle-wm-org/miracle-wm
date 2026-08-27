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

#include "overview_scene_override.h"

#include "animator.h"
#include "compositor_state.h"
#include "config.h"
#include "output.h"
#include "output_manager.h"
#include "shell_component_container.h"
#include "window_container.h"
#include "window_controller.h"
#include "workspace.h"
#include "workspace_preview.h"

#include <algorithm>
#include <cmath>
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
/// How long the overview takes to settle after a scroll or a click. Deliberately
/// snappier than the configured window animation, so that a run of scroll
/// events does not lag behind the wheel.
constexpr float RETARGET_DURATION_SECONDS = 0.18f;

/// How far the desktop background shrinks while the window strip is open.
constexpr float BACKGROUND_SCALE = 0.75f;

/// Every edge of the output. A surface attached to all four of them covers the
/// whole output, which is what makes it a background rather than a panel.
constexpr auto ALL_EDGES = static_cast<MirPlacementGravity>(
    mir_placement_gravity_north | mir_placement_gravity_south
    | mir_placement_gravity_east | mir_placement_gravity_west);

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

/// \p rectangle scaled about its own center, which is how the wallpaper shrinks
/// behind the window strip.
carousel_layout::Placement scaled_about_center(geom::Rectangle const& rectangle, float scale)
{
    auto placement = placement_of(rectangle);
    auto const center_x = placement.x + placement.width / 2.f;
    auto const center_y = placement.y + placement.height / 2.f;
    placement.width *= scale;
    placement.height *= scale;
    placement.x = center_x - placement.width / 2.f;
    placement.y = center_y - placement.height / 2.f;
    return placement;
}

/// The vertical scroll in \p event, measured in whole steps.
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

/// Turns a laid-out placement into what the renderer needs: the window is never
/// resized to fit its slot, it is relocated so that its real rectangle is
/// centered on the slot, and the shrink that makes it fit is carried by the
/// transformation.
///
/// \p clip is the output the placement rides on, so that a strip whose ends run
/// off the edges of one output stays on that output instead of spilling onto
/// the next one along.
SceneOverridePlacement to_scene_placement(
    carousel_layout::Placement const& current,
    geom::Rectangle const& real,
    geom::Rectangle const& clip)
{
    float const real_width = static_cast<float>(std::max(real.size.width.as_value(), 1));
    float const real_height = static_cast<float>(std::max(real.size.height.as_value(), 1));

    return SceneOverridePlacement {
        .position = { current.x + (current.width - real_width) / 2.f,
                     current.y + (current.height - real_height) / 2.f },
        .transformation = glm::scale(
            glm::mat4(1.f), glm::vec3(current.width / real_width, current.height / real_height, 1.f)),
        .opacity = current.opacity,
        .clip = clip
    };
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

bool miracle::is_overview_window(
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

bool miracle::is_background_window(
    std::shared_ptr<WindowContainer> const& container,
    WindowController& window_controller)
{
    if (!container || !container->window())
        return false;

    // Backgrounds arrive as layer-shell surfaces, which the policy always
    // allocates as shell components.
    if (!dynamic_cast<ShellComponentContainer const*>(container.get()))
        return false;

    auto const& info = window_controller.info_for(container->window().value());
    return info.depth_layer() < mir_depth_layer_application
        && (info.attached_edges() & ALL_EDGES) == ALL_EDGES;
}

bool miracle::is_panel_window(
    std::shared_ptr<WindowContainer> const& container,
    WindowController& window_controller)
{
    if (!container || !container->window())
        return false;

    if (!dynamic_cast<ShellComponentContainer const*>(container.get()))
        return false;

    if (is_background_window(container, window_controller))
        return false;

    // Menus, tooltips and popups are shell components too, but they are not
    // attached to an edge and have no business being cloned onto every tile.
    auto const& info = window_controller.info_for(container->window().value());
    return info.attached_edges() != mir_placement_gravity_center;
}

std::unique_ptr<OverviewSceneOverride> OverviewSceneOverride::create(
    OutputManager& output_manager,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<Config> const& config,
    std::function<void()>&& on_exit_started,
    std::function<void(uint32_t)>&& on_workspace_selected,
    std::function<void()>&& on_done)
{
    // Every output runs strips of its own, within its own bounds.
    std::vector<GroupInfo> groups;
    std::unordered_map<AbstractWorkspace const*, std::pair<size_t, size_t>> location_of_workspace;
    auto const focused_output = output_manager.focused();
    size_t active_group = 0;
    for (auto const& output : output_manager.outputs())
    {
        if (output->is_defunct())
            continue;

        auto const active = output->active();
        if (!active)
            continue;

        // Prefer the application zone so the strips avoid panels and docks.
        auto bounds = output->get_area();
        if (!output->get_app_zones().empty())
            bounds = output->get_app_zones().front().extents();

        GroupInfo info { .bounds = bounds, .source = output->get_area() };
        auto const group = groups.size();
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace == active)
                info.active_workspace = info.workspaces.size();

            location_of_workspace[workspace.get()] = { group, info.workspaces.size() };
            info.workspaces.push_back(WorkspaceSlot {
                .workspace = workspace, .id = workspace->id(), .key = workspace.get() });
        }

        if (info.workspaces.empty())
            continue;

        if (output == focused_output)
            active_group = group;

        groups.push_back(std::move(info));
    }

    if (groups.empty())
        return nullptr;

    // The focus order is already most-recently-used first, which is the order
    // the window strip wants - the focused window lands at index 0 of its group
    // and therefore starts front and center - and unlike
    // [AbstractWorkspace::for_each_window] it does not filter out floating and
    // plugin-managed windows.
    std::vector<Entry> entries;
    std::vector<ShellEntry> shell_entries;
    std::vector<size_t> next_index(groups.size(), 0);
    for (auto const& weak : compositor_state->windows())
    {
        auto const container = weak.lock();

        bool const is_background = is_background_window(container, *window_controller);
        if (is_background || is_panel_window(container, *window_controller))
        {
            auto const output = container->get_output();
            if (!output)
                continue;

            // Match the furniture to its output by area, because the group list
            // skips defunct and workspace-less outputs.
            auto const it = std::ranges::find_if(groups, [&](GroupInfo const& info)
            {
                return info.source == output->get_area();
            });
            if (it == groups.end())
                continue;

            auto const window = container->window().value();
            shell_entries.push_back(ShellEntry {
                .window = window,
                .real = geom::Rectangle { window.top_left(), window.size() },
                .group = static_cast<size_t>(std::distance(groups.begin(), it)),
                .is_background = is_background
            });
            continue;
        }

        if (!is_overview_window(container, *window_controller))
            continue;

        // A miss means the window lives on a workspace of an output that is not
        // part of the overview.
        auto const it = location_of_workspace.find(container->get_workspace().get());
        if (it == location_of_workspace.end())
            continue;

        auto const [group, workspace] = it->second;
        auto const window = container->window().value();
        entries.push_back(Entry {
            .window = window,
            .real = geom::Rectangle { window.top_left(), window.size() },
            .group = group,
            .workspace = workspace,
            .window_index = workspace == groups[group].active_workspace
                ? std::optional<size_t>(next_index[group]++)
                : std::nullopt
        });
    }

    if (entries.empty())
        return nullptr;

    return std::unique_ptr<OverviewSceneOverride>(new OverviewSceneOverride(
        std::move(entries),
        std::move(shell_entries),
        std::move(groups),
        active_group,
        animator,
        window_controller,
        compositor_state,
        config,
        std::move(on_exit_started),
        std::move(on_workspace_selected),
        std::move(on_done)));
}

OverviewSceneOverride::OverviewSceneOverride(
    std::vector<Entry> entries,
    std::vector<ShellEntry> shell_entries,
    std::vector<GroupInfo> groups,
    size_t active_group,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<Config> const& config,
    std::function<void()>&& on_exit_started,
    std::function<void(uint32_t)>&& on_workspace_selected,
    std::function<void()>&& on_done) :
    state { std::make_shared<State>() },
    groups { std::move(groups) },
    animator { animator },
    window_controller { window_controller },
    compositor_state { compositor_state },
    preview { std::make_shared<WorkspacePreview>() },
    on_exit_started { std::move(on_exit_started) },
    on_workspace_selected { std::move(on_workspace_selected) },
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

    state->groups.resize(this->groups.size());
    state->active_group = active_group;
    for (size_t group = 0; group < this->groups.size(); ++group)
    {
        state->groups[group].workspaces.count = this->groups[group].workspaces.size();
        state->groups[group].workspaces.position = this->groups[group].active_workspace;
        state->groups[group].exit_workspace = this->groups[group].active_workspace;
    }

    for (auto& entry : entries)
    {
        // The intro slides each window in from wherever it really is.
        entry.from = entry.current = placement_of(entry.real);
        if (entry.window_index)
            state->groups[entry.group].windows.count++;

        auto const* key = surface_key(entry.window);
        state->order.push_back(key);
        state->entries.emplace(key, std::move(entry));
    }

    for (auto& entry : shell_entries)
    {
        // The furniture starts where it really is, once per tile, so that the
        // element-wise lerp has something to start from at every level.
        entry.current.assign(this->groups[entry.group].workspaces.size(), placement_of(entry.real));
        entry.from = entry.target = entry.current;
        state->shell_entries.emplace(surface_key(entry.window), std::move(entry));
    }

    for (size_t group = 0; group < this->groups.size(); ++group)
        retarget(group);

    // The window strip draws one desktop. Everything the other workspaces own -
    // their copies of the furniture, and the windows that live on them - starts
    // parked where the workspace strip will pick it up, invisible, rather than
    // flying out of the desktop and fading as it goes. The workspace strip fades
    // it in from there.
    //
    // This only applies to the intro: a later move to a zero opacity target is a
    // fade out, and [retarget] rebasing `from` on `current` is what carries it.
    for (auto& [key, entry] : state->entries)
    {
        if (entry.target.opacity == 0.f)
            entry.from = entry.current = entry.target;
    }

    for (auto& [key, entry] : state->shell_entries)
    {
        for (size_t i = 0; i < entry.target.size() && i < entry.from.size(); ++i)
        {
            if (entry.target[i].opacity == 0.f)
                entry.from[i] = entry.current[i] = entry.target[i];
        }
    }
}

OverviewSceneOverride::~OverviewSceneOverride()
{
    animator->remove_by_animation_handle(animation_handle);

    // The preview is deliberately not released here: this destructor may run on
    // the animator thread, and concealing a workspace is window management. Every
    // exit path releases it on the window management thread before getting here.
}

std::vector<mir::scene::Surface const*> OverviewSceneOverride::window_strip_keys(size_t group) const
{
    std::vector<mir::scene::Surface const*> keys;
    for (auto const& [key, entry] : state->entries)
    {
        if (entry.group == group && entry.window_index)
            keys.push_back(key);
    }

    std::ranges::sort(keys, [this](auto const* a, auto const* b)
    {
        return *state->entries.at(a).window_index < *state->entries.at(b).window_index;
    });
    return keys;
}

std::optional<miral::Window> OverviewSceneOverride::centered_window(size_t group) const
{
    if (group >= state->groups.size())
        return std::nullopt;

    auto const position = state->groups[group].windows.position;
    for (auto const& [key, entry] : state->entries)
    {
        if (entry.group == group && entry.window_index == position)
            return entry.window;
    }

    return std::nullopt;
}

OverviewSceneOverride::Strip& OverviewSceneOverride::current_strip(size_t group)
{
    return state->level == Level::workspaces
        ? state->groups[group].workspaces
        : state->groups[group].windows;
}

std::vector<carousel_layout::Placement> OverviewSceneOverride::tiles_of(size_t group) const
{
    auto const& info = groups[group];
    std::vector<geom::Rectangle> const sources(info.workspaces.size(), info.source);
    return carousel_layout::compute(
        info.bounds,
        sources,
        static_cast<float>(state->groups[group].workspaces.position),
        carousel_layout::workspace_options);
}

void OverviewSceneOverride::retarget(size_t group)
{
    // Every entry - not just this group's - restarts from where it is right
    // now, because re-appending on the animation handle cancels the animation
    // that the other groups may still be part way through.
    for (auto& [key, entry] : state->entries)
        entry.from = entry.current;
    for (auto& [key, entry] : state->shell_entries)
        entry.from = entry.current;

    auto const& info = groups[group];
    auto const tiles = tiles_of(group);

    // The tiles are needed at both levels: the workspace strip lays windows into
    // them, and the window strip parks the windows of the other workspaces in
    // them at zero opacity, so that changing level is a plain fade.
    bool const showing_workspaces = state->level == Level::workspaces;
    for (auto& [key, entry] : state->entries)
    {
        if (entry.group != group)
            continue;

        if (showing_workspaces || !entry.window_index)
        {
            entry.target = carousel_layout::fit(info.source, tiles[entry.workspace], entry.real);
            if (!showing_workspaces)
                entry.target.opacity = 0.f;
        }
    }

    if (!showing_workspaces)
    {
        auto const keys = window_strip_keys(group);
        std::vector<geom::Rectangle> reals;
        reals.reserve(keys.size());
        for (auto const* key : keys)
            reals.push_back(state->entries.at(key).real);

        auto const slots = carousel_layout::compute(
            info.bounds, reals, static_cast<float>(state->groups[group].windows.position));
        for (size_t i = 0; i < keys.size(); ++i)
            state->entries.at(keys[i]).target = slots[i];
    }

    for (auto& [key, entry] : state->shell_entries)
    {
        if (entry.group != group)
            continue;

        entry.target.clear();
        entry.target.reserve(tiles.size());
        for (size_t i = 0; i < tiles.size(); ++i)
        {
            if (showing_workspaces)
            {
                entry.target.push_back(carousel_layout::fit(info.source, tiles[i], entry.real));
                continue;
            }

            // On the window strip there is only one desktop, so only the active
            // workspace's copy is drawn: the wallpaper shrinks behind the strip
            // and the panel stays exactly where it really is.
            if (i != info.active_workspace)
            {
                auto placement = carousel_layout::fit(info.source, tiles[i], entry.real);
                placement.opacity = 0.f;
                entry.target.push_back(placement);
                continue;
            }

            entry.target.push_back(entry.is_background
                    ? scaled_about_center(entry.real, BACKGROUND_SCALE)
                    : placement_of(entry.real));
        }
    }
}

bool OverviewSceneOverride::advance(size_t group, int delta)
{
    auto& strip = current_strip(group);
    if (strip.count == 0)
        return false;

    // The strip has ends: running off either one simply stops there.
    auto const last = static_cast<long>(strip.count) - 1;
    auto const next = std::clamp(static_cast<long>(strip.position) + delta, 0L, last);
    if (next == static_cast<long>(strip.position))
        return false;

    strip.position = static_cast<size_t>(next);
    retarget(group);
    return true;
}

void OverviewSceneOverride::step(int delta)
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

void OverviewSceneOverride::settle()
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

void OverviewSceneOverride::start()
{
    auto const s = state;
    animate([s]
    {
        std::lock_guard lock(s->mutex);
        if (s->phase == Phase::intro)
            s->phase = Phase::idle;
    });
}

void OverviewSceneOverride::animate(std::function<void()> on_complete, std::optional<float> duration)
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

            for (auto& [key, entry] : s->shell_entries)
            {
                entry.current.resize(entry.target.size());
                for (size_t i = 0; i < entry.target.size() && i < entry.from.size(); ++i)
                    entry.current[i] = carousel_layout::lerp(entry.from[i], entry.target[i], p);
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

void OverviewSceneOverride::nudge(miral::Window const& window)
{
    // Re-applying the current alpha notifies the surface observers
    // unconditionally, which marks the scene as damaged so the compositor
    // redraws with the new placements. (There is no way to query the current
    // transformation, so alpha is the value-preserving choice.)
    if (auto const surface = window.operator std::shared_ptr<mir::scene::Surface>())
        surface->set_alpha(surface->alpha());
}

std::optional<std::pair<size_t, size_t>> OverviewSceneOverride::locate(
    std::shared_ptr<WindowContainer> const& container) const
{
    if (!container)
        return std::nullopt;

    auto const workspace = container->get_workspace();
    if (!workspace)
        return std::nullopt;

    for (size_t group = 0; group < groups.size(); ++group)
    {
        auto const& slots = groups[group].workspaces;
        for (size_t i = 0; i < slots.size(); ++i)
        {
            if (slots[i].key == workspace.get())
                return std::pair { group, i };
        }
    }

    return std::nullopt;
}

void OverviewSceneOverride::enter_workspaces()
{
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;
        if (state->level == Level::workspaces)
            return;
    }

    // Forcing the other workspaces back into the scene is window management, and
    // input handlers run on the window management thread, so this is the one
    // place it can happen. It is deferred until now so that a user who only ever
    // taps once never pays for it.
    bool needs_reveal;
    {
        std::lock_guard lock(state->mutex);
        needs_reveal = !state->revealed;
    }

    if (needs_reveal)
    {
        // Un-hiding a window can hand it the focus, so remember what had it.
        if (auto const focused = compositor_state->focused_container())
            focus_before_reveal = focused->window();

        std::vector<std::shared_ptr<AbstractWorkspace>> to_reveal;
        for (auto const& info : groups)
        {
            for (auto const& slot : info.workspaces)
            {
                if (auto const workspace = slot.workspace.lock())
                    to_reveal.push_back(workspace);
            }
        }

        preview->acquire(to_reveal);
    }

    {
        std::lock_guard lock(state->mutex);
        state->level = Level::workspaces;

        if (needs_reveal)
        {
            state->revealed = true;

            std::vector<std::vector<carousel_layout::Placement>> tiles;
            tiles.reserve(groups.size());
            for (size_t group = 0; group < groups.size(); ++group)
                tiles.push_back(tiles_of(group));

            // A window that was hidden had its geometry captured when the
            // overview was entered, which is only as good as whatever the WM
            // last did to it. Now that it is back in the scene, re-read it.
            for (auto& [key, entry] : state->entries)
            {
                if (entry.workspace == groups[entry.group].active_workspace)
                    continue;

                entry.real = geom::Rectangle { entry.window.top_left(), entry.window.size() };
            }

            // Anything the reveal put into the scene that has no entry yet - a
            // window that appeared on a hidden workspace, say - joins now,
            // invisible in the tile it is about to fade into.
            for (auto const& weak : compositor_state->windows())
            {
                auto const container = weak.lock();
                if (!is_overview_window(container, *window_controller))
                    continue;

                auto const window = container->window().value();
                auto const* key = surface_key(window);
                if (state->entries.contains(key))
                    continue;

                auto const location = locate(container);
                if (!location)
                    continue;

                auto const [group, workspace] = *location;
                geom::Rectangle const real { window.top_left(), window.size() };
                auto placement = carousel_layout::fit(groups[group].source, tiles[group][workspace], real);
                placement.opacity = 0.f;

                state->order.push_back(key);
                state->entries.emplace(key, Entry { .window = window, .real = real, .from = placement, .target = placement, .current = placement, .group = group, .workspace = workspace });
            }
        }

        for (size_t group = 0; group < groups.size(); ++group)
            retarget(group);
    }

    settle();
}

void OverviewSceneOverride::return_to_windows()
{
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;
        if (state->level == Level::windows)
            return;

        state->level = Level::windows;
        for (size_t group = 0; group < groups.size(); ++group)
            retarget(group);
    }

    settle();
}

void OverviewSceneOverride::commit_and_exit()
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

void OverviewSceneOverride::commit_workspace_and_exit()
{
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        auto const group = state->active_group;
        auto const position = state->groups[group].workspaces.position;
        if (position >= groups[group].workspaces.size())
            return;

        // The outro grows this tile until it fills the output, so this is where
        // every entry on this output has to land - and everything that is not on
        // it fades away instead of flying home.
        state->groups[group].exit_workspace = position;
        selected_workspace = groups[group].workspaces[position].id;
    }

    begin_exit();
}

void OverviewSceneOverride::begin_exit()
{
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        state->phase = Phase::outro;
        for (auto& [key, entry] : state->entries)
        {
            entry.from = entry.current;

            // A window on a workspace the overview is not landing on is about
            // to be hidden, so it fades out where it is rather than flying home
            // across the workspace that is growing to fill the screen.
            if (entry.workspace != state->groups[entry.group].exit_workspace)
            {
                entry.target = entry.current;
                entry.target.opacity = 0.f;
                continue;
            }

            // The WM kept managing windows while the overview was open, so
            // re-read the live geometry as the outro target for a seamless
            // landing. Opacity goes back to 1 so the dimmed windows brighten as
            // they settle.
            entry.target = placement_of(geom::Rectangle { entry.window.top_left(), entry.window.size() });
        }

        for (auto& [key, entry] : state->shell_entries)
        {
            entry.from = entry.current;
            entry.target.assign(entry.current.size(), placement_of(entry.real));
            for (size_t i = 0; i < entry.target.size(); ++i)
            {
                if (i != state->groups[entry.group].exit_workspace)
                    entry.target[i].opacity = 0.f;
            }
        }
    }

    // The overview no longer owns the desktop from here on, even though the
    // outro is still playing. The phase guard above makes this fire exactly
    // once.
    on_exit_started();

    auto const s = state;
    auto const done = on_done;
    auto const held_preview = preview;
    auto const controller = window_controller;
    auto const selected = selected_workspace;
    auto const select = on_workspace_selected;
    animate([s, done, held_preview, controller, selected, select]
    {
        // This runs on the animator thread, and both adopting and concealing a
        // workspace are window management, so they go back through the window
        // management lock.
        controller->invoke_under_lock([s, held_preview, selected, select]
        {
            {
                // [cancel] may have torn the overview down while this callback
                // was waiting for the window management lock, in which case the
                // workspaces have already been put away and switching now would
                // act on a decision the user no longer has on screen.
                std::lock_guard lock(s->mutex);
                if (s->phase == Phase::done)
                    return;
            }

            // The zoom has already brought the chosen workspace up to fill the
            // output, so the switch must not animate: it only has to adopt what
            // is on screen, and it takes the focus with it.
            if (selected)
                select(*selected);

            // Whatever was just adopted is the active workspace now, so it stays
            // in the scene and only the rest goes back into hiding.
            held_preview->release();

            // Held until here so that the final outro placements keep serving
            // while the switch happens: dropping them any earlier would flash a
            // frame of the old desktop at full opacity.
            std::lock_guard lock(s->mutex);
            s->phase = Phase::done;
        });
        done();
    });
}

void OverviewSceneOverride::conceal_workspaces()
{
    preview->release();

    // The windows that just went back into hiding may have taken the focus with
    // them on the way in, which would leave it on something that is no longer in
    // the scene.
    if (focus_before_reveal)
        window_controller->select_active_window(*focus_before_reveal);

    std::lock_guard lock(state->mutex);
    state->revealed = false;
}

void OverviewSceneOverride::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const keysym = miral::toolkit::mir_keyboard_event_keysym(event);

    bool const showing_workspaces = [this]
    {
        std::lock_guard lock(state->mutex);
        return state->level == Level::workspaces;
    }();

    if (action == mir_keyboard_action_down && keysym == XKB_KEY_Escape)
    {
        // Escape backs out one level, and out of the overview entirely from the
        // window strip.
        if (showing_workspaces)
            return_to_windows();
        else
            commit_and_exit();
        return;
    }

    // The arrow keys walk the current strip along, exactly as the scroll wheel
    // does. Repeats count, so holding an arrow down runs along the strip.
    if ((action == mir_keyboard_action_down || action == mir_keyboard_action_repeat)
        && (keysym == XKB_KEY_Left || keysym == XKB_KEY_Right))
    {
        primary_tap_latched = false;
        step(keysym == XKB_KEY_Right ? 1 : -1);
        return;
    }

    // Tapping the primary action modifier again drops into the workspace strip,
    // and from there takes whatever workspace is centered.
    bool const is_primary = is_modifier_keysym(primary_modifier, keysym);
    if (action == mir_keyboard_action_down)
        primary_tap_latched = is_primary;
    else if (action == mir_keyboard_action_up && is_primary && primary_tap_latched)
    {
        primary_tap_latched = false;
        if (showing_workspaces)
            commit_workspace_and_exit();
        else
            enter_workspaces();
    }
}

void OverviewSceneOverride::handle_pointer_event(MirPointerEvent const* event)
{
    float const x = miral::toolkit::mir_pointer_event_axis_value(event, mir_pointer_axis_x);
    float const y = miral::toolkit::mir_pointer_event_axis_value(event, mir_pointer_axis_y);
    float const vscroll = vertical_scroll(event);
    auto const action = miral::toolkit::mir_pointer_event_action(event);

    if (action == mir_pointer_action_button_down)
        primary_tap_latched = false;

    bool needs_animation = false;
    bool commit = false;
    bool showing_workspaces = false;
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        showing_workspaces = state->level == Level::workspaces;

        // Scrolling and dismissal act on the output the cursor is over.
        for (size_t group = 0; group < groups.size(); ++group)
        {
            if (groups[group].bounds.contains(geom::Point { static_cast<int>(x), static_cast<int>(y) }))
            {
                state->active_group = group;
                break;
            }
        }

        if (vscroll != 0.f)
        {
            auto& strip = current_strip(state->active_group);
            strip.scroll_accumulator += vscroll;
            auto const steps = static_cast<int>(strip.scroll_accumulator);
            if (steps != 0)
            {
                strip.scroll_accumulator -= static_cast<float>(steps);

                // A positive vertical scroll is a scroll down, which advances
                // the strip to the next slot.
                needs_animation |= advance(state->active_group, steps);
            }
        }

        if (action == mir_pointer_action_button_down)
        {
            std::optional<size_t> hit_index;
            if (showing_workspaces)
            {
                // Workspace tiles are hit-tested rather than the windows in
                // them, so that clicking anywhere on a workspace picks it.
                auto const tiles = tiles_of(state->active_group);
                for (size_t i = 0; i < tiles.size(); ++i)
                {
                    if (carousel_layout::contains(tiles[i], x, y))
                    {
                        hit_index = i;
                        break;
                    }
                }
            }
            else
            {
                for (auto const* key : state->order)
                {
                    auto const it = state->entries.find(key);
                    if (it == state->entries.end() || !it->second.window_index)
                        continue;
                    if (carousel_layout::contains(it->second.current, x, y))
                    {
                        state->active_group = it->second.group;
                        hit_index = it->second.window_index;
                        break;
                    }
                }
            }

            auto& strip = current_strip(state->active_group);
            if (hit_index && *hit_index != strip.position)
            {
                // Something off to one side was picked: bring it front and
                // center rather than dismissing.
                strip.scroll_accumulator = 0.f;
                needs_animation |= advance(
                    state->active_group, static_cast<int>(*hit_index) - static_cast<int>(strip.position));
            }
            else
            {
                // What is centered, or empty space: take what is centered.
                commit = true;
            }
        }
    }

    if (commit)
    {
        if (showing_workspaces)
            commit_workspace_and_exit();
        else
            commit_and_exit();
        return;
    }

    if (needs_animation)
        settle();
}

void OverviewSceneOverride::place(
    mir::scene::Surface const& surface,
    geom::Rectangle const& real,
    std::vector<SceneOverridePlacement>& out)
{
    auto const* key = &surface;
    std::lock_guard lock(state->mutex);

    // Once the overview is finished the scene belongs to the policy again, even
    // if the override has not been released yet.
    if (state->phase == Phase::done)
        return;

    // Every placement is clipped to the output its group describes: the strips
    // deliberately run their ends off the edges of that output, and without the
    // clip the overflow would be drawn a second time by the neighbouring
    // output's renderer.
    if (auto const it = state->entries.find(key); it != state->entries.end())
    {
        out.push_back(to_scene_placement(it->second.current, real, groups[it->second.group].source));
        return;
    }

    if (auto const it = state->shell_entries.find(key); it != state->shell_entries.end())
    {
        auto const& clip = groups[it->second.group].source;
        for (auto const& placement : it->second.current)
            out.push_back(to_scene_placement(placement, real, clip));
    }
}

void OverviewSceneOverride::handle_window_added(miral::Window const& window)
{
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        auto const* key = surface_key(window);
        if (state->entries.contains(key))
            return;

        geom::Rectangle const real { window.top_left(), window.size() };

        auto const location = locate(window_controller->get_window_container(window));
        if (!location)
            return;

        auto const [group, workspace] = *location;
        bool const on_active_workspace = workspace == groups[group].active_workspace;
        if (on_active_workspace)
        {
            // The new window joins at the front of the window strip, which
            // shifts every other window on that output along by one. Shifting
            // the position by one too keeps whatever the user was looking at
            // front and center.
            for (auto& [k, e] : state->entries)
            {
                if (e.group == group && e.window_index)
                    ++*e.window_index;
            }

            state->groups[group].windows.count++;
            state->groups[group].windows.position++;
        }

        // A window on the active workspace slides in from where it really is. One
        // on any other workspace starts invisible in its own tile instead, so
        // that the window strip never shows it and the workspace strip fades it
        // in from the right place.
        auto placement = placement_of(real);
        if (!on_active_workspace)
        {
            placement = carousel_layout::fit(groups[group].source, tiles_of(group)[workspace], real);
            placement.opacity = 0.f;
        }

        state->entries.emplace(key, Entry { .window = window, .real = real, .from = placement, .target = placement, .current = placement, .group = group, .workspace = workspace, .window_index = on_active_workspace ? std::optional<size_t>(0) : std::nullopt });
        state->order.insert(state->order.begin(), key);

        // Recompute the affected output's strips so the new window glides in and
        // the existing windows shuffle around it. Other outputs keep their
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

void OverviewSceneOverride::handle_output_changed()
{
    cancel();
}

void OverviewSceneOverride::cancel()
{
    bool was_exiting = false;
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::done)
            return;

        was_exiting = state->phase == Phase::outro;
        state->phase = Phase::done;
    }

    // There is no outro to hand the workspaces back at the end of, so put them
    // away now. Cancellation always reaches us on the window management thread.
    conceal_workspaces();

    // [begin_exit] has already announced the exit if the outro was running.
    if (!was_exiting)
        on_exit_started();

    animator->remove_by_animation_handle(animation_handle);
    on_done();
}

void OverviewSceneOverride::handle_window_closed(miral::Window const& window)
{
    size_t group = 0;
    bool empty = false;
    {
        std::lock_guard lock(state->mutex);
        if (state->phase == Phase::outro || state->phase == Phase::done)
            return;

        auto const* key = surface_key(window);
        state->shell_entries.erase(key);

        auto const it = state->entries.find(key);
        if (it == state->entries.end())
            return;

        group = it->second.group;
        auto const index = it->second.window_index;
        state->entries.erase(it);
        std::erase(state->order, key);

        if (index)
        {
            // Close the gap the window left behind, and keep the position
            // pointing at a window that still exists.
            for (auto& [k, e] : state->entries)
            {
                if (e.group == group && e.window_index && *e.window_index > *index)
                    --*e.window_index;
            }

            auto& strip = state->groups[group].windows;
            strip.count--;
            if (strip.count == 0)
                strip.position = 0;
            else if (strip.position > *index || strip.position >= strip.count)
                strip.position--;
        }

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
