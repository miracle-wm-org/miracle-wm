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

#ifndef CAROUSEL_SCENE_OVERRIDE_H
#define CAROUSEL_SCENE_OVERRIDE_H

#include "animation.h"
#include "carousel_layout.h"
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
class OutputManager;
class WindowContainer;
class WindowController;

/// Returns true when \p keysym is one of the keys that produce \p modifier
/// (e.g. Super_L/Super_R for meta).
bool is_modifier_keysym(MirInputEventModifier modifier, unsigned int keysym);

/// True when \p container should get a slot in the carousel.
///
/// Shell components (panels, docks) and scratchpad-parked windows have no
/// workspace and never join the carousel. Tiled leaves always join. Floating
/// and plugin-managed windows join only when they are toplevel, so that
/// dialogs, menus and tooltips stay visually attached to whatever spawned them
/// instead of riding around as slots of their own.
bool is_carousel_window(
    std::shared_ptr<WindowContainer> const& container,
    WindowController& window_controller);

/// A carousel-style scene override: the windows on the workspace are laid out
/// side by side, with the focused one front and center at full opacity and its
/// neighbours progressively smaller and dimmer as they run off both edges of
/// the screen.
///
/// Scrolling rotates the carousel, and clicking a window that is off to one
/// side brings it to the center. Clicking the centered window (or pressing
/// Escape, or tapping the primary action modifier again) focuses whatever is
/// centered and dismisses the carousel.
class CarouselSceneOverride : public SceneOverride
{
public:
    struct Entry
    {
        miral::Window window;
        /// The window's real geometry when the carousel was entered.
        mir::geometry::Rectangle real;
        /// Where the entry is animating from / to.
        carousel_layout::Placement from;
        carousel_layout::Placement target;
        /// What [place] currently serves; written by the animation tick.
        carousel_layout::Placement current;
        /// Index into the bounds list of the output this window rides on.
        size_t group = 0;
        /// Position within that output's carousel.
        size_t index = 0;
    };

    /// One output's carousel.
    struct Group
    {
        /// How many windows ride this carousel.
        size_t count = 0;
        /// The index that is currently front and center.
        size_t position = 0;
        /// Vertical scroll that has not yet added up to a whole step.
        float scroll_accumulator = 0.f;
    };

    /// Builds a carousel of every eligible window on every output's active
    /// workspace, laid out within that output's bounds.
    ///
    /// \p on_exit_started runs on the calling (main) thread as soon as the
    /// carousel starts going away, whether or not an exit animation follows.
    /// \p on_done runs when the override may be released, which for an animated
    /// exit is on the animator thread.
    ///
    /// \returns the override, or nullptr if there is nothing to show.
    static std::unique_ptr<CarouselSceneOverride> create(
        OutputManager& output_manager,
        std::shared_ptr<Animator> const& animator,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CompositorState> const& compositor_state,
        std::shared_ptr<Config> const& config,
        std::function<void()>&& on_exit_started,
        std::function<void()>&& on_done);

    ~CarouselSceneOverride() override;

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
    CarouselSceneOverride(
        std::vector<Entry> entries,
        std::vector<mir::geometry::Rectangle> bounds,
        size_t active_group,
        std::shared_ptr<Animator> const& animator,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CompositorState> const& compositor_state,
        std::shared_ptr<Config> const& config,
        std::function<void()>&& on_exit_started,
        std::function<void()>&& on_done);

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
        /// One carousel per output, indexed by [Entry::group].
        std::vector<Group> groups;
        /// The carousel that scrolling and dismissal act on.
        size_t active_group = 0;
    };

    /// Animates every entry from its `from` placement to its `target`
    /// placement, then runs \p on_complete (on the animator thread).
    ///
    /// \p duration overrides the configured animation duration, which the
    /// short retargeting animations of a scroll or a click use so that the
    /// carousel keeps up with the input.
    void animate(std::function<void()> on_complete, std::optional<float> duration = std::nullopt);

    /// Recomputes \p group's targets for its current position and rebases every
    /// entry's animation on wherever it happens to be right now.
    ///
    /// The caller must hold `state->mutex`, and must run [animate] afterwards.
    void retarget(size_t group);

    /// The keys of \p group's entries, ordered by [Entry::index].
    ///
    /// The caller must hold `state->mutex`.
    std::vector<mir::scene::Surface const*> keys_of(size_t group) const;

    /// The window that is front and center on \p group, if any.
    ///
    /// The caller must hold `state->mutex`.
    std::optional<miral::Window> centered_window(size_t group) const;

    /// Focuses whatever is front and center on the active carousel, then exits.
    void commit_and_exit();
    void begin_exit();
    /// Tears the override down immediately, without an exit animation.
    void cancel();
    static void nudge(miral::Window const& window);

    std::shared_ptr<State> state;
    /// One carousel bounds rectangle per output, indexed by [Entry::group].
    std::vector<mir::geometry::Rectangle> const bounds;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<CompositorState> compositor_state;
    std::function<void()> on_exit_started;
    std::function<void()> on_done;
    AnimationHandle animation_handle;
    AnimationDefinition definition;
    MirInputEventModifier primary_modifier;
    bool primary_tap_latched = false;
};
}

#endif
