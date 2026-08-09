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

#ifndef SPREAD_SCENE_OVERRIDE_H
#define SPREAD_SCENE_OVERRIDE_H

#include "animation.h"
#include "scene_override.h"

#include <functional>
#include <memory>
#include <mir/geometry/rectangle.h>
#include <mir_toolkit/common.h>
#include <unordered_map>
#include <vector>

namespace mir::scene
{
class Surface;
}

namespace miracle
{
class Animator;
class CompositorState;
class Config;
class WindowController;

/// Returns true when \p keysym is one of the keys that produce \p modifier
/// (e.g. Super_L/Super_R for meta).
bool is_modifier_keysym(MirInputEventModifier modifier, unsigned int keysym);

/// An exposé-style scene override: every window on the workspace is spread
/// radially outward so they can all be seen at once. Hovering a window focuses
/// it, clicking a window focuses it and exits, and Escape (or tapping the
/// primary action modifier again) exits.
class SpreadSceneOverride : public SceneOverride
{
public:
    struct Entry
    {
        miral::Window window;
        std::weak_ptr<mir::scene::Surface> surface;
        /// The window's real geometry when the spread was entered.
        mir::geometry::Rectangle real;
        /// Where the entry is animating from / to.
        mir::geometry::Rectangle from;
        mir::geometry::Rectangle target;
        /// What [place] currently serves; written by the animation tick.
        mir::geometry::Rectangle current;
        /// Index into the bounds list of the output this window spreads on.
        size_t group = 0;
    };

    SpreadSceneOverride(
        std::vector<Entry> entries,
        std::vector<mir::geometry::Rectangle> bounds,
        std::shared_ptr<Animator> const& animator,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CompositorState> const& compositor_state,
        std::shared_ptr<Config> const& config,
        std::function<void()>&& on_done);
    ~SpreadSceneOverride() override;

    /// Begins the intro animation. Call once, after the override has been
    /// successfully installed on the [SceneOverrideManager].
    void start();

    void handle_keyboard_event(MirKeyboardEvent const* event) override;
    void handle_pointer_event(MirPointerEvent const* event) override;
    std::optional<SceneOverridePlacement> place(miral::Window const& window) override;
    void handle_window_added(miral::Window const& window) override;
    void handle_window_closed(miral::Window const& window) override;
    void handle_output_changed() override;

private:
    enum class Phase
    {
        intro,
        idle,
        outro,
        done
    };

    /// All mutable state lives here so that animation callbacks can hold a
    /// weak reference to it instead of `this`, making it safe to destroy the
    /// override on any thread while a tick is in flight.
    struct State
    {
        std::mutex mutex;
        Phase phase = Phase::intro;
        float t = 0.f;
        /// Hit-test order: most recently used first.
        std::vector<mir::scene::Surface const*> order;
        std::unordered_map<mir::scene::Surface const*, Entry> entries;
    };

    /// Animates every entry from its `from` rect to its `target` rect,
    /// then runs \p on_complete (on the animator thread).
    void animate(std::function<void()> on_complete);
    void begin_exit();
    /// Tears the override down immediately, without an exit animation.
    void cancel();
    static void nudge(std::shared_ptr<mir::scene::Surface> const& surface);

    std::shared_ptr<State> state;
    /// One spread bounds rectangle per output, indexed by [Entry::group].
    std::vector<mir::geometry::Rectangle> const bounds;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<CompositorState> compositor_state;
    std::function<void()> on_done;
    AnimationHandle animation_handle;
    AnimationDefinition definition;
    MirInputEventModifier primary_modifier;
    bool primary_tap_latched = false;
};
}

#endif
